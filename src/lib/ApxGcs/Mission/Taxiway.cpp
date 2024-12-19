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
#include "Taxiway.h"
#include "UnitMission.h"
#include <App/App.h>

Taxiway::Taxiway(MissionGroup *parent)
    : MissionItem(parent, "t#", "", "")
{
    //title
    connect(this, &Taxiway::distanceChanged, this, &Taxiway::updateTitle);
    updateTitle();

    App::jsync(this);
}

void Taxiway::updateTitle()
{
    QStringList st;
    st.append(QString::number(num() + 1));
    st.append(AppRoot::distanceToString(distance()));
    setTitle(st.join(' '));
}

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
    setBearing(azimuth);
    return p;
}
