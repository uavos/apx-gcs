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
#include "Taxiway.h"
#include "VehicleMission.h"
#include <App/App.h>
//=============================================================================
Taxiway::Taxiway(MissionGroup *parent)
    : MissionItem(parent, "t#", "", tr("Taxiway"))
{
    //title
    connect(this, &Taxiway::distanceChanged, this, &Taxiway::updateTitle);
    updateTitle();

    App::jsync(this);
}
//=============================================================================
void Taxiway::updateTitle()
{
    QStringList st;
    st.append(QString::number(num() + 1));
    st.append(AppRoot::distanceToString(distance()));
    setTitle(st.join(' '));
}
//=============================================================================
QGeoPath Taxiway::getPath()
{
    QGeoPath p;
    const double spd = 5;
    MissionItem *prev = prevItem();
    double distance = 0;
    double azimuth = 0;
    if (prev) {
        QGeoCoordinate p1(prev->coordinate());
        QGeoCoordinate p2(coordinate());
        p.addCoordinate(p1);
        p.addCoordinate(p2);
        distance = p1.distanceTo(p2);
        azimuth = p1.azimuthTo(p2);
    }
    //update properties
    setDistance(distance);
    setTime(distance / spd);
    setCourse(azimuth);
    return p;
}
//=============================================================================
