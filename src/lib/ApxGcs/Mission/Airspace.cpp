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
#include "Airspace.h"

Airspace::Airspace(UnitMission *parent)
    : Fact(parent, "airspace", "Airspace", tr("Geofences and NoFly zones"), Group | ModifiedGroup)
{
    // f_add = new Fact(this, "add_object", tr("Add Geofence"), "", Action, "plus-circle");

    // Fact *f;
    // f = new Fact(f_add,
    //              "add_circle",
    //              tr("Circle"),
    //              tr("Add circular geofence"),
    //              CloseOnTrigger,
    //              "circle-outline");

    // f = new Fact(f_add,
    //              "add_polygon",
    //              tr("Polygon"),
    //              tr("Add polygon geofence"),
    //              CloseOnTrigger,
    //              "vector-polygon");

    // f = new Fact(f_add,
    //              "add_line",
    //              tr("Line"),
    //              tr("Add line geofence"),
    //              CloseOnTrigger,
    //              "vector-line");
}

void Airspace::add(const QGeoCoordinate &p, Shape shape)
{
    // auto item = new AirspaceItem(this);
    // item->setCoordinate(p);
    // item->setShape(shape);
    // item->setSelected(true);
}
