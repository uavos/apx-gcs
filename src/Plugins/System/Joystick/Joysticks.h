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
class JoystickConfig;

class Joysticks : public Fact
{
    Q_OBJECT
public:
    Joysticks(Fact *parent = nullptr);

    Fact *f_enabled;

    // user configurations, edited without devices
    Fact *f_configs;
    Fact *f_new;
    Fact *f_newTitle;
    Fact *f_newName;
    Fact *f_newCopy;
    Fact *f_create;
    Fact *f_user;

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

    int legacyConfigIndex(const QString &uid);
    int userConfigIndex(const QString &title, const QString &name);
    // the same, but the device name must match too, even when it is empty
    int userConfigIndexExact(const QString &title, const QString &name);

    void updateConfEnums();
    void updateConfigEditors();
    // show the config, which matches the controller, in its selection box
    void showConfig(Joystick *j);

    void configEdited(JoystickConfig *c);
    void configRemove(JoystickConfig *c);

private slots:
    void updateStatus();
    void updateEnabled();

    void deviceAdded(JoystickDeviceInfo info);
    void deviceRemoved(int id, QString error);
    void axisChanged(int id, int axis, qreal value);
    void buttonChanged(int id, int button, bool pressed);
    void hatChanged(int id, int hat, quint8 mask);

    void loadConfigs();
    void saveConfigs();

    void exportConfigs();
    void importConfigs();

    void createConfig();
};
