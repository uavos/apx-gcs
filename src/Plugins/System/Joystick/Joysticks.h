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

#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>

#include "JoystickBackend.h"
class Joystick;

class Joysticks : public Fact
{
    Q_OBJECT
public:
    Joysticks(Fact *parent = nullptr);

    Fact *f_enabled;

    Fact *f_configs;

    Fact *f_list;

    Fact *f_export;
    Fact *f_import;

private:
    JoystickBackend *backend{nullptr};
    QMap<int, Joystick *> map; // connected controllers by backend device id

    QList<QJsonObject> configs;
    QList<QString> configIds;
    QMap<QString, int> configTitles;

    DelayedEvent saveEvent;

    bool _updatingEnums{false};

    Joystick *addJoystick(const JoystickDeviceInfo &info, int device_index);

    int userConfigIndex(const QString &title, const QString &name);

    void updateConfEnums();

private slots:
    void updateStatus();
    void updateEnabled();

    void deviceAdded(JoystickDeviceInfo info);
    void deviceRemoved(int id);
    void axisChanged(int id, int axis, qreal value);
    void buttonChanged(int id, int button, bool pressed);
    void hatChanged(int id, int hat, quint8 mask);

    void loadConfigs();
    void saveConfigs();

    void exportConfigs();
    void importConfigs();
};
