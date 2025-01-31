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
#include "MissionTools.h"
#include "Poi.h"
#include "Runway.h"
#include "Taxiway.h"
#include "UnitMission.h"
#include "Waypoint.h"

#include <App/App.h>
#include <Fleet/Fleet.h>
#include <Fleet/UnitSelect.h>

MissionTools::MissionTools(UnitMission *mission, Flags flags)
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
    f_altadjust = new Fact(f, "delta", tr("Value to add"), "", Int);
    f_altadjust->setUnits("m");
    f_altadjust->setIcon(f->icon());
    connect(f_altadjust, &Fact::valueChanged, this, [this]() {
        f_altadjustApply->setEnabled(f_altadjust->value().toInt() != 0);
    });
    f_altadjustApply = new Fact(f,
                                "apply",
                                tr("Apply"),
                                "",
                                Action | Apply | CloseOnTrigger | ShowDisabled);
    f_altadjustApply->setEnabled(false);
    connect(f_altadjustApply, &Fact::triggered, this, &MissionTools::altadjustTriggered);

    f = new Fact(this, "altset", tr("Altitude set"), tr("Set all waypoints altitude"), Group);
    f->setIcon("format-align-middle");
    connect(f, &Fact::triggered, this, &MissionTools::updateMaxAltitude);
    f_altset = new Fact(f, "altitude", tr("Altitude value"), "", Int);
    f_altset->setUnits("m");
    f_altset->setIcon(f->icon());
    f_altset->setMin(0);
    connect(f_altset, &Fact::valueChanged, this, [this]() {
        f_altsetApply->setEnabled(f_altset->value().toInt() != 0);
    });
    f_altsetApply = new Fact(f,
                             "apply",
                             tr("Apply"),
                             "",
                             Action | Apply | CloseOnTrigger | ShowDisabled);
    f_altsetApply->setEnabled(false);
    connect(f_altsetApply, &Fact::triggered, this, &MissionTools::altsetTriggered);

    f = new Fact(this, "aglset", tr("AGL set"), tr("Set all waypoints height AGL"), Group);
    f->setIcon("arrow-expand-vertical");
    f->setVisible(false);
    connect(f, &Fact::triggered, this, &MissionTools::updateMaxAltitude);
    f_aglset = new Fact(f, "agl", tr("AGL value"), "", Int);
    f_aglset->setUnits("m");
    f_aglset->setIcon(f->icon());
    f_aglset->setMin(0);
    connect(f_aglset, &Fact::valueChanged, this, [this]() {
        f_aglsetApply->setEnabled(f_aglset->value().toInt() != 0);
    });
    f_aglsetApply = new Fact(f,
                             "apply",
                             tr("Apply"),
                             "",
                             Action | Apply | CloseOnTrigger | ShowDisabled);
    f_aglsetApply->setEnabled(false);

    auto fvs = new UnitSelect(this, "copy", tr("Copy"), tr("Copy to unit"));
    f_copy = fvs;
    f_copy->setIcon("content-copy");
    connect(fvs, &UnitSelect::unitSelected, this, &MissionTools::copyUnitSelected);
}

void MissionTools::altadjustTriggered()
{
    int v = f_altadjust->value().toInt();
    for (int i = 0; i < mission->f_waypoints->size(); ++i) {
        Fact *f = static_cast<Waypoint *>(mission->f_waypoints->child(i))->f_altitude;
        f->setValue(f->value().toInt() + v);
    }
    f_altadjust->setValue(0);
}

void MissionTools::altsetTriggered()
{
    int v = f_altset->value().toInt();
    for (int i = 0; i < mission->f_waypoints->size(); ++i) {
        Fact *f = static_cast<Waypoint *>(mission->f_waypoints->child(i))->f_altitude;
        f->setValue(v);
    }
}

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

void MissionTools::copyUnitSelected(Unit *unit)
{
    if (unit == mission->unit)
        return;
    unit->f_mission->fromJson(mission->toJson());
    Fleet::instance()->selectUnit(unit);
}

