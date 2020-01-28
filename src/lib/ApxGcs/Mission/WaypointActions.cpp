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
#include "WaypointActions.h"
#include "MissionField.h"
#include "Waypoint.h"
//=============================================================================
WaypointActions::WaypointActions(Waypoint *parent)
    : Fact(parent, "actions", tr("Actions"), tr("Actions to perform on waypoint"), Group)
    , blockActionsValueChanged(false)
{
    setObjectName("actions"); //name to identify in text value
    f_speed = new MissionField(this,
                               "speed",
                               tr("Speed"),
                               tr("Fly with this speed to waypoint"),
                               Int);
    f_speed->setEnumStrings(QStringList() << "cruise");
    f_speed->setUnits("m/s");
    f_speed->setMin(0);
    f_speed->setMax(1000);
    f_shot = new MissionField(this, "shot", tr("Shot"), tr("Make a cam shot on waypoint"), Enum);
    f_shot->setEnumStrings(QStringList() << "no"
                                         << "single"
                                         << "start"
                                         << "stop");
    f_dshot = new MissionField(this,
                               "dshot",
                               tr("Auto Shot"),
                               tr("Continuous cam shots distance"),
                               Int);
    f_dshot->setEnumStrings(QStringList() << "no");
    f_dshot->setUnits("m");
    f_dshot->setMin(0);

    f_script = new MissionField(this,
                                "script",
                                tr("Script"),
                                tr("Execute VM script (@function) on waypoint"),
                                Text);
    f_poi = new MissionField(this, "poi", tr("POI"), tr("Linked Point Of Interest"), Int);
    f_poi->setEnumStrings(QStringList() << "off");
    f_poi->setMin(0);
    f_loiter = new MissionField(this,
                                "loiter",
                                tr("Loiter"),
                                tr("Loiter around POI or waypoint"),
                                Enum);
    f_loiter->setEnumStrings(QStringList() << "no"
                                           << "yes");
    f_turnR = new MissionField(this, "radius", tr("Radius"), tr("Loiter radius"), Int);
    f_turnR->setEnumStrings(QStringList() << "default");
    f_turnR->setUnits("m");
    f_loops = new MissionField(this, "loops", tr("Loops"), tr("Loiter loops limit"), Int);
    f_loops->setEnumStrings(QStringList() << "default");
    f_loops->setMin(0);
    f_time = new MissionField(this, "time", tr("Time"), tr("Loiter time limit"), Int);
    f_time->setEnumStrings(QStringList() << "default");
    f_time->setUnits("time");
    f_time->setMin(0);
    f_time->setMax(60 * 60 * 24);
    connect(this, &Fact::valueChanged, this, &WaypointActions::actionsValueChanged);
    updateActionsValue();

    for (int i = 0; i < size(); ++i) {
        connect(child(i), &Fact::valueChanged, this, &WaypointActions::updateActionsValue);
    }
}
//=============================================================================
void WaypointActions::hashData(QCryptographicHash *h) const
{
    for (int i = 0; i < size(); ++i) {
        child(i)->hashData(h);
    }
}
//=============================================================================
void WaypointActions::updateActionsValue()
{
    QStringList st;
    /*for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        if (f->isZero())
            continue;
        st.append(f->title());
    }
    setStatusText(st.join(','));
    st.clear();*/
    for (int i = 0; i < this->size(); ++i) {
        Fact *f = this->child(i);
        if (f->isZero())
            continue;
        st.append(QString("%1=%2").arg(f->name()).arg(f->text()));
    }
    blockActionsValueChanged = true;
    this->setValue(st.join(','));
    blockActionsValueChanged = false;
}
void WaypointActions::actionsValueChanged()
{
    if (blockActionsValueChanged)
        return;
    QMap<QString, QString> map;
    foreach (const QString &s, this->value().toString().split(',')) {
        int i = s.indexOf('=');
        if (i <= 0)
            continue;
        map[s.left(i)] = s.mid(i + 1);
    }
    foreach (const QString &s, map.keys()) {
        Fact *f = child(s);
        if (!f) {
            qDebug() << "Waypoint action data key not found" << s;
            continue;
        }
        f->setValue(map.value(s));
    }
}
//=============================================================================
