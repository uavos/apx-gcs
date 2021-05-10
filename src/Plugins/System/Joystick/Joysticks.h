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

#include <SDL.h>
#include <QtConcurrent>
#include <QtCore>

#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
class Joystick;

class Joysticks : public Fact
{
    Q_OBJECT
public:
    Joysticks(Fact *parent = nullptr);

    Fact *f_enabled;

    Fact *f_configs;

    Fact *f_list;

private:
    QTimer timer;
    QMap<int, Joystick *> map;

    QList<QJsonObject> configs;
    QList<QString> configIds;
    QMap<QString, int> configTitles;

    DelayedEvent saveEvent;

    Joystick *addJoystick(int device_index, QString uid);

    int configIndex(const QJsonObject &config);

    void updateConfEnums();

    QFutureWatcher<int> watcher;
    void waitEvent();

    void processEvent(const SDL_Event &event);

private slots:
    void updateStatus();
    void updateEnabled();
    void watcherFinished();

    void update();
    void scan();

    void loadConfigs();
    void saveConfigs();
};
