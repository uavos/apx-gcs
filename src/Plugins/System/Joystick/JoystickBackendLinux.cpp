/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Linux joystick backend: kernel evdev interface (/dev/input/event*).
// No external libraries: open/ioctl/read on device nodes,
// QSocketNotifier for input and QFileSystemWatcher for hotplug.
//
// Device detection, controls numbering and values scaling are ported
// from SDL2 2.30.8 (see the notice in JoystickBackend.cpp):
//   src/joystick/linux/SDL_sysjoystick.c
//   src/core/linux/SDL_evdev_capabilities.c
// The origin of each part is noted next to the code.

#if defined(__linux__) && !defined(__ANDROID__)

#include "JoystickBackend.h"

#include <QFileSystemWatcher>
#include <QSocketNotifier>

#include <algorithm>
#include <cerrno>
#include <cmath>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr int BITS_PER_LONG = sizeof(unsigned long) * 8;
constexpr int nlongs(int bits)
{
    return (bits + BITS_PER_LONG - 1) / BITS_PER_LONG;
}
inline bool testBit(const unsigned long *a, int bit)
{
    return (a[bit / BITS_PER_LONG] >> (bit % BITS_PER_LONG)) & 1UL;
}

const char *const INPUT_DIR = "/dev/input";
constexpr int SCAN_DELAY_MS = 300; // let udev set permissions of a new node
constexpr int SCAN_RETRIES = 6;
constexpr int HATS_MAX = 4; // ABS_HAT0X..ABS_HAT3Y

struct Axis
{
    int code;
    int min;
    int max;
    float scale;
    JoystickAxisFilter filter;
};

// Digital hat: a pair of axes
struct Hat
{
    bool present{false};
    int index{-1};     // hat number of the device
    int axis[2]{1, 1}; // position by X and Y: 0 - negative, 1 - center, 2 - positive
    int min[2]{-1, -1};
    int max[2]{1, 1};
    quint8 pos{JoystickBackend::HatCentered};
};

struct Device
{
    int id{-1};
    int fd{-1};
    QString path;
    QSocketNotifier *notifier{nullptr};
    bool dropped{false}; // kernel buffer overrun, waiting for resync

    QVector<Axis> axes;
    QVector<int> buttonCodes;
    QVector<bool> buttonStates;
    Hat hats[HATS_MAX]; // by the number of axes pair
    int nhats{0};

    // event code to index lookup, -1 when not used
    int axisIndex[ABS_CNT];
    int keyIndex[KEY_CNT];
};

struct Event
{
    enum Type { AxisEvent, ButtonEvent, HatEvent };
    Type type;
    int index;
    int value;
};
using Events = QVector<Event>;

// SDL_EVDEV_GuessDeviceClass (SDL_evdev_capabilities.c), the joystick part.
// SDL uses it when the device class is not provided by udev.
bool isJoystick(const unsigned long *evbit, const unsigned long *keybit, const unsigned long *absbit)
{
    if (!testBit(evbit, EV_ABS))
        return false;

    // X, Y, Z axes but no buttons probably means an accelerometer
    if (testBit(absbit, ABS_X) && testBit(absbit, ABS_Y) && testBit(absbit, ABS_Z)
        && !testBit(evbit, EV_KEY))
        return false;

    // RX, RY, RZ axes but no buttons also probably means an accelerometer
    if (testBit(absbit, ABS_RX) && testBit(absbit, ABS_RY) && testBit(absbit, ABS_RZ)
        && !testBit(evbit, EV_KEY))
        return false;

    if (testBit(absbit, ABS_X) && testBit(absbit, ABS_Y)) {
        if (testBit(keybit, BTN_TRIGGER) || testBit(keybit, BTN_A) || testBit(keybit, BTN_1)
            || testBit(absbit, ABS_RX) || testBit(absbit, ABS_RY) || testBit(absbit, ABS_RZ)
            || testBit(absbit, ABS_THROTTLE) || testBit(absbit, ABS_RUDDER)
            || testBit(absbit, ABS_WHEEL) || testBit(absbit, ABS_GAS)
            || testBit(absbit, ABS_BRAKE))
            return true;
    }
    return false;
}

// GuessIfAxesAreDigitalHat (SDL_sysjoystick.c)
// A "hat" is a digital input with at most 9 possible states, the kernel makes
// no distinction between digital hat axes and continuous axes, so guess.
bool isDigitalHat(const struct input_absinfo *x, const struct input_absinfo *y)
{
    // If both axes are missing, they're not anything
    if (!x && !y)
        return false;

    // If both axes have ranges constrained between -1 and 1, they're definitely digital
    if ((!x || (x->minimum == -1 && x->maximum == 1))
        && (!y || (y->minimum == -1 && y->maximum == 1)))
        return true;

    // If both axes lack fuzz, flat, and resolution values, they're probably digital
    if ((!x || (!x->fuzz && !x->flat && !x->resolution))
        && (!y || (!y->fuzz && !y->flat && !y->resolution)))
        return true;

    // Otherwise, treat them as analog
    return false;
}

// AxisCorrect (SDL_sysjoystick.c), the default mode without deadzones
int axisCorrect(const Axis &a, int value)
{
    if (a.min != a.max) {
        value = static_cast<int>(
            std::floor((value - a.min) * a.scale + JoystickBackend::AxisMin + 0.5f));
    }
    // Clamp and return
    if (value < JoystickBackend::AxisMin)
        return JoystickBackend::AxisMin;
    if (value > JoystickBackend::AxisMax)
        return JoystickBackend::AxisMax;
    return value;
}

} // namespace

class JoystickBackendLinux : public JoystickBackend
{
public:
    explicit JoystickBackendLinux(QObject *parent)
        : JoystickBackend(parent)
    {
        scanTimer.setSingleShot(true);
        scanTimer.setInterval(SCAN_DELAY_MS);
        connect(&scanTimer, &QTimer::timeout, this, [this]() { scan(); });

        connect(&watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
            scanRetries = SCAN_RETRIES;
            scanTimer.start();
        });
    }
    ~JoystickBackendLinux() override { stop(); }

    bool start() override
    {
        if (started)
            return true;
        if (!QDir(INPUT_DIR).exists())
            return false;
        started = true;
        watcher.addPath(INPUT_DIR);
        scanRetries = 0;
        scan();
        return true;
    }

    void stop() override
    {
        if (!started)
            return;
        started = false;
        scanTimer.stop();
        watcher.removePath(INPUT_DIR);
        const auto list = devices.values();
        for (auto dev : list)
            closeDevice(dev, false);
    }

private:
    QFileSystemWatcher watcher;
    QTimer scanTimer;
    int scanRetries{0};
    int nextId{0};
    bool started{false};

    QHash<QString, Device *> devices; // by device node path

    // Compare device nodes with the opened devices list
    void scan()
    {
        if (!started)
            return;

        QStringList names = QDir(INPUT_DIR).entryList(QStringList() << "event*",
                                                      QDir::AllEntries | QDir::System);
        auto number = [](const QString &s) { return s.mid(5).toInt(); };
        std::sort(names.begin(), names.end(), [number](const QString &a, const QString &b) {
            return number(a) < number(b);
        });

        QSet<QString> present;
        bool denied = false;
        for (const auto &name : names) {
            if (!started)
                return; // stopped from a signal handler
            const QString path = QString(INPUT_DIR) + '/' + name;
            present.insert(path);
            if (devices.contains(path))
                continue;
            openDevice(path, &denied);
        }
        if (!started)
            return;

        const auto list = devices.values();
        for (auto dev : list) {
            if (present.contains(dev->path))
                continue;
            if (devices.value(dev->path) != dev)
                continue; // already closed from a signal handler
            closeDevice(dev, true);
        }

        // a new node could be not accessible yet, permissions are set by udev later
        if (denied && scanRetries > 0 && started) {
            scanRetries--;
            scanTimer.start();
        }
    }

    void openDevice(const QString &path, bool *denied)
    {
        int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            if (errno == EACCES || errno == EPERM)
                *denied = true;
            return;
        }

        // IsJoystick (SDL_sysjoystick.c)
        unsigned long evbit[nlongs(EV_CNT)] = {};
        unsigned long keybit[nlongs(KEY_CNT)] = {};
        unsigned long absbit[nlongs(ABS_CNT)] = {};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0
            || ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0
            || ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) < 0) {
            ::close(fd);
            return;
        }
        if (!isJoystick(evbit, keybit, absbit)) {
            ::close(fd);
            return;
        }

        struct input_id inpid = {};
        char product_string[128] = {};
        if (ioctl(fd, EVIOCGID, &inpid) < 0
            || ioctl(fd, EVIOCGNAME(sizeof(product_string) - 1), product_string) < 0) {
            ::close(fd);
            return;
        }
        if (ignoredDevice(inpid.vendor, inpid.product)) {
            ::close(fd);
            return;
        }

        // FixupDeviceInfoForMapping (SDL_sysjoystick.c): affects the device id only
        if (inpid.vendor == 0x045e && inpid.product == 0x0b05 && inpid.version == 0x0903) {
            // Microsoft Xbox One Elite Series 2 controller,
            // the first version of the firmware duplicated all the inputs
            if (testBit(keybit, 0x2c0))
                inpid.version = 0x0902;
        }
        if (inpid.vendor == 0x3250 && (inpid.product == 0x1001 || inpid.product == 0x1002)) {
            // Atari vcs modern and classic controllers
            inpid.version = 0;
        }

        auto dev = new Device;
        dev->fd = fd;
        dev->path = path;
        std::fill(std::begin(dev->axisIndex), std::end(dev->axisIndex), -1);
        std::fill(std::begin(dev->keyIndex), std::end(dev->keyIndex), -1);

        // ConfigJoystick (SDL_sysjoystick.c): numbering of controls

        // buttons: joystick and gamepad codes first, then all the others
        auto addButton = [dev, &keybit](int code) {
            if (!testBit(keybit, code))
                return;
            dev->keyIndex[code] = dev->buttonCodes.size();
            dev->buttonCodes.append(code);
            dev->buttonStates.append(false);
        };
        for (int code = BTN_JOYSTICK; code < KEY_MAX; ++code)
            addButton(code);
        for (int code = 0; code < BTN_JOYSTICK; ++code)
            addButton(code);

        // hats: pairs of axes which look digital
        for (int code = ABS_HAT0X; code <= ABS_HAT3Y; code += 2) {
            int hat_x = -1;
            int hat_y = -1;
            struct input_absinfo absinfo_x = {};
            struct input_absinfo absinfo_y = {};
            if (testBit(absbit, code))
                hat_x = ioctl(fd, EVIOCGABS(code), &absinfo_x);
            if (testBit(absbit, code + 1))
                hat_y = ioctl(fd, EVIOCGABS(code + 1), &absinfo_y);
            if (!isDigitalHat(hat_x < 0 ? nullptr : &absinfo_x, hat_y < 0 ? nullptr : &absinfo_y))
                continue;
            Hat &hat = dev->hats[(code - ABS_HAT0X) / 2];
            hat.present = true;
            hat.index = dev->nhats++;
            hat.min[0] = (hat_x < 0) ? -1 : absinfo_x.minimum;
            hat.max[0] = (hat_x < 0) ? 1 : absinfo_x.maximum;
            hat.min[1] = (hat_y < 0) ? -1 : absinfo_y.minimum;
            hat.max[1] = (hat_y < 0) ? 1 : absinfo_y.maximum;
        }

        // axes: in the order of codes, digital hats are skipped
        for (int code = 0; code < ABS_MAX; ++code) {
            if (code >= ABS_HAT0X && code <= ABS_HAT3Y
                && dev->hats[(code - ABS_HAT0X) / 2].present)
                continue;
            if (!testBit(absbit, code))
                continue;
            struct input_absinfo absinfo = {};
            if (ioctl(fd, EVIOCGABS(code), &absinfo) < 0)
                continue;
            Axis a = {};
            a.code = code;
            a.min = absinfo.minimum;
            a.max = absinfo.maximum;
            if (a.min != a.max) {
                const float value_range = (a.max - a.min);
                const float output_range = (AxisMax - AxisMin);
                a.scale = (output_range / value_range);
            }
            dev->axisIndex[code] = dev->axes.size();
            dev->axes.append(a);
        }

        JoystickDeviceInfo info;
        info.id = dev->id = nextId++;
        info.name = makeName(inpid.vendor,
                             inpid.product,
                             QString(),
                             QString::fromUtf8(product_string));
        info.uid = makeUid(inpid.bustype,
                           inpid.vendor,
                           inpid.product,
                           inpid.version,
                           QByteArray(),
                           QByteArray(product_string));
        info.axes = dev->axes.size();
        info.buttons = dev->buttonCodes.size();
        info.hats = dev->nhats;

        dev->notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(dev->notifier, &QSocketNotifier::activated, this, [this, dev]() {
            readDevice(dev);
        });

        devices.insert(path, dev);

        // Report the device and the current state of its controls, as SDL does
        // for a just opened joystick: pressed buttons and hats are reported,
        // axes are reported when moved.
        Events events;
        pollAllValues(dev, events);
        const int id = dev->id;
        emit deviceAdded(info);
        dispatch(id, events);
    }

    void closeDevice(Device *dev, bool notify)
    {
        devices.remove(dev->path);
        if (dev->notifier) {
            dev->notifier->setEnabled(false);
            dev->notifier->deleteLater();
        }
        ::close(dev->fd);
        const int id = dev->id;
        delete dev;
        if (notify)
            emit deviceRemoved(id);
    }

    // HandleInputEvents (SDL_sysjoystick.c)
    void readDevice(Device *dev)
    {
        // Signals are emitted after the device data is processed:
        // a handler may stop the backend and destroy the device.
        const int id = dev->id;
        const QString path = dev->path;
        Events events;
        bool gone = false;

        struct input_event ev[32];
        for (;;) {
            const ssize_t n = ::read(dev->fd, ev, sizeof(ev));
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    gone = true; // device is unplugged
                break;
            }
            if (n == 0) {
                gone = true;
                break;
            }
            const int cnt = static_cast<int>(n / sizeof(struct input_event));
            for (int i = 0; i < cnt; ++i)
                processEvent(dev, ev[i], events);
        }

        dispatch(id, events);

        if (!gone)
            return;
        if (devices.value(path) != dev)
            return; // already closed from a signal handler
        closeDevice(dev, true);
        // the node may be replaced by a new one
        scanRetries = SCAN_RETRIES;
        scanTimer.start();
    }

    void processEvent(Device *dev, const struct input_event &e, Events &events)
    {
        // If the kernel sent a SYN_DROPPED, we are supposed to ignore the
        // rest of the packet (the end of it signified by a SYN_REPORT)
        if (dev->dropped && (e.type != EV_SYN || e.code != SYN_REPORT))
            return;

        switch (e.type) {
        case EV_KEY:
            if (e.code < KEY_CNT)
                setButton(dev, dev->keyIndex[e.code], e.value, events);
            break;
        case EV_ABS:
            if (e.code >= ABS_CNT)
                break;
            if (e.code >= ABS_HAT0X && e.code <= ABS_HAT3Y) {
                const int hatidx = (e.code - ABS_HAT0X) / 2;
                if (dev->hats[hatidx].present) {
                    handleHat(dev, hatidx, e.code % 2, e.value, events);
                    break;
                }
            }
            setAxis(dev, dev->axisIndex[e.code], e.value, events);
            break;
        case EV_SYN:
            switch (e.code) {
            case SYN_DROPPED:
                dev->dropped = true;
                break;
            case SYN_REPORT:
                if (dev->dropped) {
                    dev->dropped = false;
                    pollAllValues(dev, events); // try to sync up to current state now
                }
                break;
            }
            break;
        }
    }

    // PollAllValues (SDL_sysjoystick.c)
    void pollAllValues(Device *dev, Events &events)
    {
        // Poll all axis
        for (int i = 0; i < dev->axes.size(); ++i) {
            struct input_absinfo absinfo = {};
            if (ioctl(dev->fd, EVIOCGABS(dev->axes.at(i).code), &absinfo) >= 0)
                setAxis(dev, i, absinfo.value, events);
        }

        // Poll all digital hats
        for (int code = ABS_HAT0X; code <= ABS_HAT3Y; code++) {
            const int baseaxis = code - ABS_HAT0X;
            const int hatidx = baseaxis / 2;
            if (!dev->hats[hatidx].present)
                continue;
            struct input_absinfo absinfo = {};
            if (ioctl(dev->fd, EVIOCGABS(code), &absinfo) >= 0)
                handleHat(dev, hatidx, baseaxis % 2, absinfo.value, events);
        }

        // Poll all buttons
        unsigned long keyinfo[nlongs(KEY_CNT)] = {};
        if (ioctl(dev->fd, EVIOCGKEY(sizeof(keyinfo)), keyinfo) >= 0) {
            for (int i = 0; i < dev->buttonCodes.size(); ++i)
                setButton(dev, i, testBit(keyinfo, dev->buttonCodes.at(i)) ? 1 : 0, events);
        }
    }

    void setAxis(Device *dev, int index, int raw, Events &events)
    {
        if (index < 0 || index >= dev->axes.size())
            return;
        Axis &a = dev->axes[index];
        QVarLengthArray<int, 2> out;
        a.filter.update(axisCorrect(a, raw), out);
        for (int v : out)
            events.append(Event{Event::AxisEvent, index, v});
    }

    // SDL_PrivateJoystickButton (SDL_joystick.c)
    void setButton(Device *dev, int index, int state, Events &events)
    {
        if (state != 0 && state != 1)
            return; // invalid state (key autorepeat)
        if (index < 0 || index >= dev->buttonStates.size())
            return;
        const bool pressed = state == 1;
        if (dev->buttonStates.at(index) == pressed)
            return;
        dev->buttonStates[index] = pressed;
        events.append(Event{Event::ButtonEvent, index, state});
    }

    // HandleHat (SDL_sysjoystick.c), deadzones are enabled as by default in SDL
    void handleHat(Device *dev, int hatidx, int axis, int value, Events &events)
    {
        static const quint8 position_map[3][3] = {
            {HatLeft | HatUp, HatUp, HatRight | HatUp},
            {HatLeft, HatCentered, HatRight},
            {HatLeft | HatDown, HatDown, HatRight | HatDown},
        };

        Hat &hat = dev->hats[hatidx];
        // The deadzones prevent the slightest of twitches on an analog axis
        // from registering as a hat movement. If the axes really are digital,
        // this won't hurt since they only ever send min, 0, or max anyway.
        if (value < 0) {
            if (value <= hat.min[axis]) {
                hat.min[axis] = value;
                value = 0;
            } else if (value < hat.min[axis] / 3) {
                value = 0;
            } else {
                value = 1;
            }
        } else if (value > 0) {
            if (value >= hat.max[axis]) {
                hat.max[axis] = value;
                value = 2;
            } else if (value > hat.max[axis] / 3) {
                value = 2;
            } else {
                value = 1;
            }
        } else { // value == 0
            value = 1;
        }
        if (value == hat.axis[axis])
            return;
        hat.axis[axis] = value;

        // SDL_PrivateJoystickHat (SDL_joystick.c)
        const quint8 pos = position_map[hat.axis[1]][hat.axis[0]];
        if (pos == hat.pos)
            return;
        hat.pos = pos;
        events.append(Event{Event::HatEvent, hat.index, pos});
    }

    void dispatch(int id, const Events &events)
    {
        for (const auto &e : events) {
            switch (e.type) {
            case Event::AxisEvent:
                emit axisChanged(id, e.index, axisValue(e.value));
                break;
            case Event::ButtonEvent:
                emit buttonChanged(id, e.index, e.value != 0);
                break;
            case Event::HatEvent:
                emit hatChanged(id, e.index, static_cast<quint8>(e.value));
                break;
            }
        }
    }
};

JoystickBackend *JoystickBackend::create(QObject *parent)
{
    return new JoystickBackendLinux(parent);
}

#endif
