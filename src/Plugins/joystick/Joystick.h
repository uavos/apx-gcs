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
#ifndef Joystick_H
#define Joystick_H
#include <QtCore>

#include <ApxMisc/DelayedEvent.h>
#include <Fact/Fact.h>
#include <SDL.h>
//=============================================================================
class Joystick : public Fact
{
    Q_OBJECT
public:
    explicit Joystick(Fact *parent, int device_index, QString uid);
    ~Joystick();

    int device_index;
    QString uid;
    SDL_JoystickID instanceID;
    QString devName;

    Fact *f_conf;

    Fact *f_title;

    Fact *f_axes;
    Fact *f_buttons;
    Fact *f_hats;

    Fact *f_save;

    QString juid() const;

    void updateDevice(bool connected);
    void updateAxis(int i, qreal v);
    void updateButton(int i, bool v);
    void updateHat(int i, quint8 v);

    void loadConfig(const QJsonObject &config);
    QJsonObject saveConfig();

private:
    SDL_Joystick *dev;
    DelayedEvent saveEvent;

signals:
    void save();
};
//=============================================================================
#endif
