/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef Joysticks_H
#define Joysticks_H
#include <SDL.h>
#include <QtConcurrent>
#include <QtCore>

#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
class Joystick;
//=============================================================================
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

    void update();
    void scan();

    void loadConfigs();
    void saveConfigs();
};
//=============================================================================
#endif
