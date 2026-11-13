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

// macOS joystick backend: IOKit HID Manager (system framework).
// The manager is scheduled on the main run loop, which is driven by Qt,
// so all callbacks arrive in the main thread.
//
// Device detection, controls numbering and values scaling are ported
// from SDL2 2.30.8 (see the notice in JoystickBackend.cpp):
//   src/joystick/darwin/SDL_iokitjoystick.c
// The origin of each part is noted next to the code.
//
// The difference from SDL: SDL polls every control of every device on each
// update, here the values are delivered by the HID Manager when they change.

#if defined(__APPLE__)

#include "JoystickBackend.h"

#include <algorithm>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDUsageTables.h>

namespace {

constexpr quint16 HARDWARE_BUS_USB = 0x03; // SDL_HARDWARE_BUS_USB

// recElement (SDL_iokitjoystick_c.h)
struct Element
{
    enum Kind { Axis, Button, Hat };
    Kind kind;
    int index; // number of the control within its kind
    IOHIDElementRef ref;
    uint32_t usagePage;
    uint32_t usage;
    SInt32 min; // reported min value possible
    SInt32 max; // reported max value possible

    // runtime variables used for auto-calibration
    SInt32 minReport; // min returned value
    SInt32 maxReport; // max returned value

    // last reported state
    JoystickAxisFilter filter; // axis
    int state;                 // button: 0 or 1, hat: directions mask
};

struct Device
{
    int id{-1};
    IOHIDDeviceRef ref{nullptr};
    CFArrayRef elementsArray{nullptr}; // keeps element references alive
    QHash<uint32_t, Element> elements; // by element cookie
};

struct Event
{
    Element::Kind kind;
    int index;
    int value;
};
using Events = QVector<Event>;

// AddHIDElement (SDL_iokitjoystick.c): controls of interest by HID usage
bool elementKind(uint32_t usagePage, uint32_t usage, Element::Kind *kind)
{
    switch (usagePage) {
    case kHIDPage_GenericDesktop:
        switch (usage) {
        case kHIDUsage_GD_X:
        case kHIDUsage_GD_Y:
        case kHIDUsage_GD_Z:
        case kHIDUsage_GD_Rx:
        case kHIDUsage_GD_Ry:
        case kHIDUsage_GD_Rz:
        case kHIDUsage_GD_Slider:
        case kHIDUsage_GD_Dial:
        case kHIDUsage_GD_Wheel:
            *kind = Element::Axis;
            return true;
        case kHIDUsage_GD_Hatswitch:
            *kind = Element::Hat;
            return true;
        case kHIDUsage_GD_DPadUp:
        case kHIDUsage_GD_DPadDown:
        case kHIDUsage_GD_DPadRight:
        case kHIDUsage_GD_DPadLeft:
        case kHIDUsage_GD_Start:
        case kHIDUsage_GD_Select:
        case kHIDUsage_GD_SystemMainMenu:
            *kind = Element::Button;
            return true;
        }
        return false;
    case kHIDPage_Simulation:
        switch (usage) {
        case kHIDUsage_Sim_Rudder:
        case kHIDUsage_Sim_Throttle:
        case kHIDUsage_Sim_Accelerator:
        case kHIDUsage_Sim_Brake:
            *kind = Element::Axis;
            return true;
        }
        return false;
    case kHIDPage_Button:
    case kHIDPage_Consumer: // e.g. 'pause' button on Steelseries MFi gamepads
        *kind = Element::Button;
        return true;
    }
    return false;
}

// DARWIN_JoystickUpdate (SDL_iokitjoystick.c): hat switch position.
// The value is the direction number clockwise from up.
quint8 hatPosition(const Element &e, SInt32 value)
{
    const SInt32 range = (e.max - e.min + 1);
    value -= e.min;
    if (range == 4) {
        value *= 2; // 4 position hatswitch - scale up value
    } else if (range != 8) {
        value = -1; // Neither a 4 nor 8 positions - fall back to default position (centered)
    }
    switch (value) {
    case 0:
        return JoystickBackend::HatUp;
    case 1:
        return JoystickBackend::HatRight | JoystickBackend::HatUp;
    case 2:
        return JoystickBackend::HatRight;
    case 3:
        return JoystickBackend::HatRight | JoystickBackend::HatDown;
    case 4:
        return JoystickBackend::HatDown;
    case 5:
        return JoystickBackend::HatLeft | JoystickBackend::HatDown;
    case 6:
        return JoystickBackend::HatLeft;
    case 7:
        return JoystickBackend::HatLeft | JoystickBackend::HatUp;
    default:
        // Every other value is mapped to center: some joysticks use 8
        // and some 15 for this value, and there are even more variants.
        return JoystickBackend::HatCentered;
    }
}

QByteArray stringProperty(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef ref = IOHIDDeviceGetProperty(device, key);
    if (!ref || CFGetTypeID(ref) != CFStringGetTypeID())
        return QByteArray();
    // SDL reads the strings to a buffer of 256 bytes
    return QString::fromCFString(static_cast<CFStringRef>(ref)).toUtf8().left(255);
}

SInt32 numberProperty(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef ref = IOHIDDeviceGetProperty(device, key);
    if (!ref || CFGetTypeID(ref) != CFNumberGetTypeID())
        return 0;
    SInt32 v = 0;
    CFNumberGetValue(static_cast<CFNumberRef>(ref), kCFNumberSInt32Type, &v);
    return v;
}

// CreateHIDDeviceMatchDictionary (SDL_iokitjoystick.c)
CFDictionaryRef createMatching(uint32_t usagePage, uint32_t usage)
{
    CFNumberRef page = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usagePage);
    CFNumberRef use = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usage);
    const void *keys[] = {CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey)};
    const void *values[] = {page, use};
    CFDictionaryRef dict = CFDictionaryCreate(kCFAllocatorDefault,
                                              keys,
                                              values,
                                              2,
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);
    CFRelease(page);
    CFRelease(use);
    return dict;
}

} // namespace

class JoystickBackendMac : public JoystickBackend
{
public:
    explicit JoystickBackendMac(QObject *parent)
        : JoystickBackend(parent)
    {}
    ~JoystickBackendMac() override { stop(); }

    // CreateHIDManager, ConfigHIDManager (SDL_iokitjoystick.c)
    bool start() override
    {
        if (manager)
            return true;

        manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (!manager)
            return false;

        CFMutableArrayRef matching = CFArrayCreateMutable(kCFAllocatorDefault,
                                                          0,
                                                          &kCFTypeArrayCallBacks);
        for (uint32_t usage :
             {kHIDUsage_GD_Joystick, kHIDUsage_GD_GamePad, kHIDUsage_GD_MultiAxisController}) {
            CFDictionaryRef dict = createMatching(kHIDPage_GenericDesktop, usage);
            CFArrayAppendValue(matching, dict);
            CFRelease(dict);
        }
        IOHIDManagerSetDeviceMatchingMultiple(manager, matching);
        CFRelease(matching);

        IOHIDManagerRegisterDeviceMatchingCallback(manager, onDeviceMatched, this);
        IOHIDManagerRegisterDeviceRemovalCallback(manager, onDeviceRemoved, this);
        IOHIDManagerRegisterInputValueCallback(manager, onInputValue, this);

        // connected devices are reported by the matching callback from the run loop
        IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), kCFRunLoopCommonModes);

        if (IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
            stop();
            return false;
        }
        return true;
    }

    void stop() override
    {
        if (!manager)
            return;

        IOHIDManagerRef m = manager;
        manager = nullptr;

        IOHIDManagerRegisterDeviceMatchingCallback(m, nullptr, nullptr);
        IOHIDManagerRegisterDeviceRemovalCallback(m, nullptr, nullptr);
        IOHIDManagerRegisterInputValueCallback(m, nullptr, nullptr);
        IOHIDManagerUnscheduleFromRunLoop(m, CFRunLoopGetMain(), kCFRunLoopCommonModes);

        const auto list = devices.values();
        devices.clear();
        for (auto dev : list)
            freeDevice(dev);

        auto destroy = [m]() {
            IOHIDManagerClose(m, kIOHIDOptionsTypeNone);
            CFRelease(m);
        };
        // The manager can't be destroyed from its own callback
        // (stop requested by a signal handler): do it from the event loop.
        if (callbackDepth > 0 && QCoreApplication::instance())
            QTimer::singleShot(0, QCoreApplication::instance(), destroy);
        else
            destroy();
    }

private:
    IOHIDManagerRef manager{nullptr};
    QHash<IOHIDDeviceRef, Device *> devices;
    int nextId{0};
    int callbackDepth{0};

    struct CallbackGuard
    {
        explicit CallbackGuard(JoystickBackendMac *b)
            : b(b)
        {
            b->callbackDepth++;
        }
        ~CallbackGuard() { b->callbackDepth--; }
        JoystickBackendMac *b;
    };

    static void onDeviceMatched(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
    {
        Q_UNUSED(sender)
        if (result != kIOReturnSuccess)
            return;
        auto self = static_cast<JoystickBackendMac *>(context);
        CallbackGuard guard(self);
        self->addDevice(device);
    }
    static void onDeviceRemoved(void *context, IOReturn result, void *sender, IOHIDDeviceRef device)
    {
        Q_UNUSED(result)
        Q_UNUSED(sender)
        auto self = static_cast<JoystickBackendMac *>(context);
        CallbackGuard guard(self);
        self->removeDevice(device);
    }
    static void onInputValue(void *context, IOReturn result, void *sender, IOHIDValueRef value)
    {
        Q_UNUSED(result)
        Q_UNUSED(sender)
        auto self = static_cast<JoystickBackendMac *>(context);
        CallbackGuard guard(self);
        self->inputValue(value);
    }

    void freeDevice(Device *dev)
    {
        if (dev->elementsArray)
            CFRelease(dev->elementsArray);
        delete dev;
    }

    // JoystickDeviceWasAddedCallback, GetDeviceInfo (SDL_iokitjoystick.c)
    void addDevice(IOHIDDeviceRef device)
    {
        if (!manager || !device)
            return;
        if (devices.contains(device))
            return; // IOKit sent us a duplicate

        // Filter device list to non-keyboard/mouse stuff
        if (numberProperty(device, CFSTR(kIOHIDPrimaryUsagePageKey)) != kHIDPage_GenericDesktop)
            return;
        const SInt32 primaryUsage = numberProperty(device, CFSTR(kIOHIDPrimaryUsageKey));
        if (primaryUsage != kHIDUsage_GD_Joystick && primaryUsage != kHIDUsage_GD_GamePad
            && primaryUsage != kHIDUsage_GD_MultiAxisController)
            return;

        const quint16 vendor = static_cast<quint16>(
            numberProperty(device, CFSTR(kIOHIDVendorIDKey)));
        const quint16 product = static_cast<quint16>(
            numberProperty(device, CFSTR(kIOHIDProductIDKey)));
        const quint16 version = static_cast<quint16>(
            numberProperty(device, CFSTR(kIOHIDVersionNumberKey)));
        const QByteArray manufacturer_string = stringProperty(device, CFSTR(kIOHIDManufacturerKey));
        const QByteArray product_string = stringProperty(device, CFSTR(kIOHIDProductKey));

        if (ignoredDevice(vendor, product))
            return;

        CFArrayRef array = IOHIDDeviceCopyMatchingElements(device, nullptr, kIOHIDOptionsTypeNone);
        if (!array)
            return;

        // AddHIDElement (SDL_iokitjoystick.c): collect input controls.
        // The array contains all elements of the device, collections are
        // followed by their children, so the order is the same as SDL gets
        // by descending into collections.
        QVector<Element> axes, buttons, hats;
        QSet<uint32_t> cookies; // ElementAlreadyAdded
        const CFIndex cnt = CFArrayGetCount(array);
        for (CFIndex i = 0; i < cnt; ++i) {
            CFTypeRef ref = CFArrayGetValueAtIndex(array, i);
            if (!ref || CFGetTypeID(ref) != IOHIDElementGetTypeID())
                continue;
            IOHIDElementRef e = static_cast<IOHIDElementRef>(const_cast<void *>(ref));

            switch (IOHIDElementGetType(e)) {
            case kIOHIDElementTypeInput_Misc:
            case kIOHIDElementTypeInput_Button:
            case kIOHIDElementTypeInput_Axis:
                break;
            default:
                continue;
            }

            const uint32_t cookie = static_cast<uint32_t>(IOHIDElementGetCookie(e));
            if (cookies.contains(cookie))
                continue;

            Element el = {};
            el.ref = e;
            el.usagePage = IOHIDElementGetUsagePage(e);
            el.usage = IOHIDElementGetUsage(e);
            if (!elementKind(el.usagePage, el.usage, &el.kind))
                continue;
            el.minReport = el.min = static_cast<SInt32>(IOHIDElementGetLogicalMin(e));
            el.maxReport = el.max = static_cast<SInt32>(IOHIDElementGetLogicalMax(e));
            cookies.insert(cookie);

            switch (el.kind) {
            case Element::Axis:
                axes.append(el);
                break;
            case Element::Button:
                buttons.append(el);
                break;
            case Element::Hat:
                hats.append(el);
                break;
            }
        }

        auto dev = new Device;
        dev->ref = device;
        dev->elementsArray = array;

        // SDL keeps each list sorted by usage: a new element is inserted
        // after all elements with the same or lower usage.
        for (auto list : {&axes, &buttons, &hats}) {
            std::stable_sort(list->begin(), list->end(), [](const Element &a, const Element &b) {
                return a.usage < b.usage;
            });
            for (int i = 0; i < list->size(); ++i) {
                Element &el = (*list)[i];
                el.index = i;
                dev->elements.insert(static_cast<uint32_t>(IOHIDElementGetCookie(el.ref)), el);
            }
        }

        JoystickDeviceInfo info;
        info.id = dev->id = nextId++;
        info.name = makeName(vendor,
                             product,
                             QString::fromUtf8(manufacturer_string),
                             QString::fromUtf8(product_string));
        info.uid = makeUid(HARDWARE_BUS_USB,
                           vendor,
                           product,
                           version,
                           manufacturer_string,
                           product_string);
        info.axes = axes.size();
        info.buttons = buttons.size();
        info.hats = hats.size();

        devices.insert(device, dev);

        // Report the device and the current state of its controls, as SDL does
        // on the first update: pressed buttons and hats are reported,
        // axes are reported when moved.
        Events events;
        for (auto it = dev->elements.begin(); it != dev->elements.end(); ++it) {
            IOHIDValueRef value = nullptr;
            if (IOHIDDeviceGetValue(device, it->ref, &value) != kIOReturnSuccess || !value)
                continue;
            applyValue(*it, value, events);
        }
        const int id = dev->id;
        // Signals are emitted after the device data is processed:
        // a handler may stop the backend and destroy the device.
        emit deviceAdded(info);
        dispatch(id, events);
    }

    void removeDevice(IOHIDDeviceRef device)
    {
        Device *dev = devices.take(device);
        if (!dev)
            return;
        const int id = dev->id;
        freeDevice(dev);
        emit deviceRemoved(id);
    }

    void inputValue(IOHIDValueRef value)
    {
        if (!value)
            return;
        IOHIDElementRef e = IOHIDValueGetElement(value);
        if (!e)
            return;
        Device *dev = devices.value(IOHIDElementGetDevice(e));
        if (!dev)
            return;
        auto it = dev->elements.find(static_cast<uint32_t>(IOHIDElementGetCookie(e)));
        if (it == dev->elements.end())
            return;

        Events events;
        applyValue(*it, value, events);
        dispatch(dev->id, events);
    }

    // DARWIN_JoystickUpdate (SDL_iokitjoystick.c)
    void applyValue(Element &el, IOHIDValueRef valueRef, Events &events)
    {
        // GetHIDElementState
        SInt32 value = static_cast<SInt32>(IOHIDValueGetIntegerValue(valueRef));
        // record min and max for auto calibration
        if (value < el.minReport)
            el.minReport = value;
        if (value > el.maxReport)
            el.maxReport = value;

        switch (el.kind) {
        case Element::Axis: {
            // GetHIDScaledCalibratedState(device, element, -32768, 32767, &value)
            const float deviceScale = AxisMax - AxisMin;
            const float readScale = el.maxReport - el.minReport;
            if (readScale != 0)
                value = ((value - el.minReport) * deviceScale / readScale) + AxisMin;

            // SDL passes the value as Sint16
            QVarLengthArray<int, 2> out;
            el.filter.update(static_cast<qint16>(value), out);
            for (int v : out)
                events.append(Event{el.kind, el.index, v});
        } break;
        case Element::Button: {
            if (value > 1)
                value = 1; // handle pressure-sensitive buttons
            // SDL_PrivateJoystickButton (SDL_joystick.c)
            if (value != 0 && value != 1)
                break; // invalid state
            if (value == el.state)
                break;
            el.state = value;
            events.append(Event{el.kind, el.index, value});
        } break;
        case Element::Hat: {
            const int pos = hatPosition(el, value);
            // SDL_PrivateJoystickHat (SDL_joystick.c)
            if (pos == el.state)
                break;
            el.state = pos;
            events.append(Event{el.kind, el.index, pos});
        } break;
        }
    }

    void dispatch(int id, const Events &events)
    {
        for (const auto &e : events) {
            switch (e.kind) {
            case Element::Axis:
                emit axisChanged(id, e.index, axisValue(e.value));
                break;
            case Element::Button:
                emit buttonChanged(id, e.index, e.value != 0);
                break;
            case Element::Hat:
                emit hatChanged(id, e.index, static_cast<quint8>(e.value));
                break;
            }
        }
    }
};

JoystickBackend *JoystickBackend::create(QObject *parent)
{
    return new JoystickBackendMac(parent);
}

#endif
