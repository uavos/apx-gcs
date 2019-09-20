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
#include "MissionTools.h"
#include "MissionStorage.h"
#include "Poi.h"
#include "Runway.h"
#include "Taxiway.h"
#include "VehicleMission.h"
#include "Waypoint.h"

#include <Vehicles/VehicleSelect.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
MissionTools::MissionTools(VehicleMission *mission, Flags flags)
    : Fact(mission, "tools", tr("Tools"), tr("Mission edit tools"), flags)
    , mission(mission)
{
    setIcon("dots-horizontal");

    Fact *f;

    f = new Fact(this,
                 "altadjust",
                 tr("Altitude adjust"),
                 tr("Adjust all waypoints altitude"),
                 Group);
    f->setIcon("altimeter");
    f_altadjust = new Fact(f, "value", tr("Value to add"), "", Int);
    f_altadjust->setUnits("m");
    f_altadjust->setIcon(f->icon());
    connect(f_altadjust, &Fact::valueChanged, this, [=]() {
        f_altadjust->setModified(false);
        f_altadjustApply->setEnabled(f_altadjust->value().toInt() != 0);
    });
    f_altadjustApply = new Fact(f, "apply", tr("Apply"), "", Action | Apply | CloseOnTrigger);
    f_altadjustApply->setEnabled(false);
    connect(f_altadjustApply, &Fact::triggered, this, &MissionTools::altadjustTriggered);

    f = new Fact(this, "altset", tr("Altitude set"), tr("Set all waypoints altitude"), Group);
    f->setIcon("format-align-middle");
    connect(f, &Fact::triggered, this, &MissionTools::updateMaxAltitude);
    f_altset = new Fact(f, "value", tr("Altitude value"), "", Int);
    f_altset->setUnits("m");
    f_altset->setIcon(f->icon());
    f_altset->setMin(0);
    connect(f_altset, &Fact::valueChanged, this, [=]() {
        f_altset->setModified(false);
        f_altsetApply->setEnabled(f_altset->value().toInt() != 0);
    });
    f_altsetApply = new Fact(f, "apply", tr("Apply"), "", Action | Apply | CloseOnTrigger);
    f_altsetApply->setEnabled(false);
    connect(f_altsetApply, &Fact::triggered, this, &MissionTools::altsetTriggered);

    VehicleSelect *fvs = new VehicleSelect(this, "copy", tr("Copy"), tr("Copy to vehicle"));
    if (!mission->vehicle->isLocal())
        fvs->addVehicle(Vehicles::instance()->f_local);
    f_copy = fvs;
    f_copy->setIcon("content-copy");
    connect(fvs, &VehicleSelect::vehicleSelected, this, &MissionTools::copyVehicleSelected);
}
//=============================================================================
void MissionTools::altadjustTriggered()
{
    int v = f_altadjust->value().toInt();
    for (int i = 0; i < mission->f_waypoints->size(); ++i) {
        Fact *f = static_cast<Waypoint *>(mission->f_waypoints->child(i))->f_altitude;
        f->setValue(f->value().toInt() + v);
    }
    f_altadjust->setValue(0);
}
//=============================================================================
void MissionTools::altsetTriggered()
{
    int v = f_altset->value().toInt();
    for (int i = 0; i < mission->f_waypoints->size(); ++i) {
        Fact *f = static_cast<Waypoint *>(mission->f_waypoints->child(i))->f_altitude;
        f->setValue(v);
    }
}
//=============================================================================
void MissionTools::updateMaxAltitude()
{
    int alt = 0;
    for (int i = 0; i < mission->f_waypoints->size(); ++i) {
        Waypoint *wp = qobject_cast<Waypoint *>(mission->f_waypoints->child(i));
        if (!wp)
            continue;
        int v = wp->f_altitude->value().toInt();
        if (alt < v)
            alt = v;
    }
    if (alt > 0)
        f_altset->setValue(alt);
}
//=============================================================================
void MissionTools::copyVehicleSelected(Vehicle *vehicle)
{
    if (vehicle == mission->vehicle)
        return;
    QString hash = mission->storage->dbHash;
    if (hash.isEmpty())
        return;
    vehicle->f_mission->storage->loadMission(hash);
    vehicle->f_mission->setModified(true, true);
    Vehicles::instance()->selectVehicle(vehicle);
}
//=============================================================================
