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
#include "MapTools.h"
#include <Vehicles/Vehicles.h>
#include <Mission/VehicleMission.h>
#include <ApxApp.h>
//=============================================================================
MapTools::MapTools(Fact *parent)
    : Fact(parent, "map", tr("Map tools"), tr("Mission planner map tools"), Group)
{
    setIcon("map");
    model()->setFlat(true);

    f_add = new Fact(this, "add", tr("Add object"), tr("Add new map object"), Section);
    f_add->setIcon("plus-circle");

    f_vehicle = new Fact(this, "vehicle", tr("Vehicle"), tr("Send vehicle command"), Section);
    f_vehicle->setIcon("drone");

    Fact *f;

    f = new Fact(f_add, "waypoint", tr("Waypoint"), "");
    f->setIcon("map-marker");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_waypoints->add(clickCoordinate()); });
    f = new Fact(f_add, "point", tr("Point of interest"), "");
    f->setIcon("map-marker-radius");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_pois->add(clickCoordinate()); });
    f = new Fact(f_add, "runway", tr("Runway"), "");
    f->setIcon("road");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_runways->add(clickCoordinate()); });
    f = new Fact(f_add, "taxiway", tr("Taxiway"), "");
    f->setIcon("vector-polyline");
    connect(f, &Fact::triggered, this, [=]() { mission()->f_taxiways->add(clickCoordinate()); });

    f = new Fact(f_vehicle, "fly", tr("Fly here"), "");
    f->setIcon("airplane");
    connect(f, &Fact::triggered, this, [=]() { vehicle()->flyHere(clickCoordinate()); });
    f = new Fact(f_vehicle, "look", tr("Look here"), "");
    f->setIcon("eye");
    connect(f, &Fact::triggered, this, [=]() { vehicle()->lookHere(clickCoordinate()); });

    f = new Fact(f_vehicle, "home", tr("Set home"), "");
    f->setIcon("home-map-marker");
    connect(f, &Fact::triggered, this, [=]() { vehicle()->setHomePoint(clickCoordinate()); });
    f = new Fact(f_vehicle, "fix", tr("Send position fix"), "");
    f->setIcon("crosshairs-gps");
    connect(f, &Fact::triggered, this, [=]() { vehicle()->sendPositionFix(clickCoordinate()); });

    for (int i = 0; i < f_add->size(); ++i) {
        connect(f_add->child(i), &Fact::triggered, this, &Fact::actionTriggered);
    }
    for (int i = 0; i < f_vehicle->size(); ++i) {
        connect(f_vehicle->child(i), &Fact::triggered, this, &Fact::actionTriggered);
    }

    qml = ApxApp::instance()->engine()->loadQml("qrc:/MapPlugin.qml");
}
MapTools::~MapTools()
{
    //qDebug()<<"del begin"<<this;
    //ApxApp::jsexec("ui.map.clearMapItems()");
    //ApxApp::jsexec("ui.map.destroy()");
    //ApxApp::jsexec("delete ui.map");
    //qml->deleteLater();
    //delete qml;
    //qDebug()<<"del"<<this;
}
//=============================================================================
//=============================================================================
Vehicle *MapTools::vehicle() const
{
    return Vehicles::instance()->current();
}
VehicleMission *MapTools::mission() const
{
    return vehicle()->f_mission;
}
//=============================================================================
//=============================================================================
//=============================================================================
QGeoCoordinate MapTools::clickCoordinate() const
{
    return m_clickCoordinate;
}
void MapTools::setClickCoordinate(const QGeoCoordinate &v)
{
    if (m_clickCoordinate == v)
        return;
    m_clickCoordinate = v;
    emit clickCoordinateChanged();
}
QGeoShape MapTools::area() const
{
    return m_area;
}
void MapTools::setArea(const QGeoShape &v)
{
    if (m_area == v)
        return;
    m_area = v;
    emit areaChanged();
}
//=============================================================================
