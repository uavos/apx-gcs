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
#include "MissionPlanner.h"
#include "MapPrefs.h"
#include <App/App.h>
#include <Fleet/Fleet.h>
#include <Mission/UnitMission.h>

MissionPlanner::MissionPlanner(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Mission Planner"),
           tr("Map view and mission editor"),
           Group | FlatModel,
           "map")
{
    f_add = new Fact(this, "add", tr("Add object"), tr("Add new map object"), Section, "plus-circle");

    f_unit = new Fact(this, "unit", tr("Unit"), tr("Send unit command"), Section, "drone");

    Fact *f;

    f = new Fact(f_add, "waypoint", tr("Waypoint"), "", CloseOnTrigger, "map-marker");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_waypoints->add(clickCoordinate()); });

    f = new Fact(f_add, "point", tr("Point of interest"), "", CloseOnTrigger, "map-marker-radius");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_pois->add(clickCoordinate()); });

    f = new Fact(f_add, "runway", tr("Runway"), "", CloseOnTrigger, "road");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_runways->add(clickCoordinate()); });

    f = new Fact(f_add, "taxiway", tr("Taxiway"), "", CloseOnTrigger, "vector-polyline");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_taxiways->add(clickCoordinate()); });

    f = new Fact(f_add, "area", tr("Area"), "", CloseOnTrigger, "vector-polygon");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_areas->add(clickCoordinate()); });

    f = new Fact(f_unit, "fly_here", tr("Fly here"), "", CloseOnTrigger, "airplane");
    connect(f, &Fact::triggered, this, [=]() { unit()->flyHere(clickCoordinate()); });

    f = new Fact(f_unit, "look_here", tr("Look here"), "", CloseOnTrigger, "eye");
    connect(f, &Fact::triggered, this, [=]() { unit()->lookHere(clickCoordinate()); });

    f = new Fact(f_unit, "home_here", tr("Set home"), "", CloseOnTrigger, "home-map-marker");
    connect(f, &Fact::triggered, this, [=]() { unit()->setHomePoint(clickCoordinate()); });

    f = new Fact(f_unit, "pos_here", tr("Fix position"), "", CloseOnTrigger, "crosshairs-gps");
    connect(f, &Fact::triggered, this, [=]() { unit()->sendPositionFix(clickCoordinate()); });

    new MapPrefs(this);

    qml = loadQml("qrc:/MissionPlannerPlugin.qml");
}

Unit *MissionPlanner::unit() const
{
    return Fleet::instance()->current();
}
UnitMission *MissionPlanner::mission() const
{
    return unit()->f_mission;
}

QGeoCoordinate MissionPlanner::clickCoordinate() const
{
    return m_clickCoordinate;
}
void MissionPlanner::setClickCoordinate(const QGeoCoordinate &v)
{
    if (m_clickCoordinate == v)
        return;
    m_clickCoordinate = v;
    emit clickCoordinateChanged();
}
QGeoShape MissionPlanner::area() const
{
    return m_area;
}
void MissionPlanner::setArea(const QGeoShape &v)
{
    if (m_area == v)
        return;
    m_area = v;
    emit areaChanged();
}
