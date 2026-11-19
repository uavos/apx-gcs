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
#pragma once

#include <QtCore>

// Description of a connected input device, reported by the backend
struct JoystickDeviceInfo
{
    int id{-1};   // unique id of the connection, never reused while the app runs
    QString name; // device title, see JoystickBackend::makeName
    QString uid;  // hardware model id, see JoystickBackend::makeUid
    int axes{0};
    int buttons{0};
    int hats{0};
};

// Axis motion filter.
// Ported from SDL2 2.30.8: SDL_PrivateJoystickAxis (src/joystick/SDL_joystick.c).
// Nothing is reported until there is a real activity on the axis,
// then the initial position is reported first.
struct JoystickAxisFilter
{
    bool has_initial_value{false};
    bool has_second_value{false};
    bool sent_initial_value{false};
    int initial_value{0};
    int value{0};

    // v is in device independent units (AxisMin..AxisMax),
    // values to report are appended to out
    void update(int v, QVarLengthArray<int, 2> &out);
};

// Platform access to joysticks.
// Implementations use OS services only and deliver everything
// in the thread of the object (signals are emitted from the Qt event loop).
//
// Device detection, controls numbering, values scaling and device naming
// follow SDL2 2.30.8, which was used by the plugin before,
// to keep saved configurations valid.
class JoystickBackend : public QObject
{
    Q_OBJECT
public:
    explicit JoystickBackend(QObject *parent = nullptr)
        : QObject(parent)
    {}

    // hat directions bitmask for hatChanged (the same values as SDL_HAT_*)
    enum HatMask : quint8 {
        HatCentered = 0,
        HatUp = 1,
        HatRight = 2,
        HatDown = 4,
        HatLeft = 8,
    };

    // device independent axis range (SDL_JOYSTICK_AXIS_MIN/MAX)
    static constexpr int AxisMin = -32768;
    static constexpr int AxisMax = 32767;

    // axis value reported by axisChanged
    static qreal axisValue(int v) { return static_cast<qreal>(v) / AxisMax; }

    // returns nullptr when the platform is not supported
    static JoystickBackend *create(QObject *parent);

    // Device title from the strings reported by hardware
    // (SDL_CreateJoystickName)
    static QString makeName(quint16 vendor,
                            quint16 product,
                            const QString &vendorName,
                            const QString &productName);

    // Hardware model id, hex string of 16 bytes
    // (SDL_CreateJoystickGUID, the same value as SDL_JoystickGetDeviceGUID returned)
    static QString makeUid(quint16 bus,
                           quint16 vendor,
                           quint16 product,
                           quint16 version,
                           const QByteArray &vendorName,
                           const QByteArray &productName);

    // Keyboards, mice and tablets which report themselves as joysticks
    // (SDL_ShouldIgnoreJoystick: blacklist_devices and rog_gamepad_mice)
    static bool ignoredDevice(quint16 vendor, quint16 product);

    // start() reports already connected devices through deviceAdded
    virtual bool start() = 0;
    // stop() closes all devices silently, without deviceRemoved
    virtual void stop() = 0;

signals:
    void deviceAdded(JoystickDeviceInfo info);
    void deviceRemoved(int id);

    void axisChanged(int id, int axis, qreal value); // value is -1..+1
    void buttonChanged(int id, int button, bool pressed);
    void hatChanged(int id, int hat, quint8 mask);
};
