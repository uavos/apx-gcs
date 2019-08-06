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
#ifndef WaypointActions_H
#define WaypointActions_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
class Waypoint;
//=============================================================================
class WaypointActions : public Fact
{
    Q_OBJECT

public:
    explicit WaypointActions(Waypoint *parent);

    Fact *f_speed;
    Fact *f_shot;
    Fact *f_dshot;
    Fact *f_script;
    Fact *f_poi;
    Fact *f_loiter;
    Fact *f_turnR;
    Fact *f_loops;
    Fact *f_time;

private:
    bool blockActionsValueChanged;

protected:
    void hashData(QCryptographicHash *h) const;

private slots:
    void updateActionsValue();
    void actionsValueChanged();
};
//=============================================================================
#endif
