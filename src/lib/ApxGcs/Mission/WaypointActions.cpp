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
#include "WaypointActions.h"
#include "MissionField.h"
#include "Waypoint.h"

WaypointActions::WaypointActions(Waypoint *parent)
    : Fact(parent,
           "actions",
           tr("Actions"),
           tr("Actions to perform on waypoint"),
           Group | ModifiedGroup)
    , blockActionsValueChanged(false)
{
    f_speed = new MissionField(this,
                               "speed",
                               tr("Speed"),
                               tr("Fly with this speed to waypoint"),
                               Int);
    f_speed->setEnumStrings(QStringList() << "off");
    f_speed->setUnits("m/s");
    f_speed->setMin(0);
    f_speed->setMax(1000);

    f_poi = new MissionField(this, "poi", tr("POI"), tr("Linked Point Of Interest"), Int);
    f_poi->setEnumStrings(QStringList() << "off");
    f_poi->setMin(0);

    f_script = new MissionField(this,
                                "script",
                                tr("Script"),
                                tr("Execute VM script (@function) on waypoint"),
                                Text);

    connect(this, &Fact::valueChanged, this, &WaypointActions::actionsValueChanged);
    updateActionsValue();

    for (int i = 0; i < size(); ++i) {
        connect(child(i), &Fact::valueChanged, this, &WaypointActions::updateActionsValue);
    }
}

void WaypointActions::hashData(QCryptographicHash *h) const
{
    for (int i = 0; i < size(); ++i) {
        child(i)->hashData(h);
    }
}

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
        st.append(QString("%1=%2").arg(f->name()).arg(f->valueText()));
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

QJsonValue WaypointActions::toJson()
{
    QJsonObject jso;
    for (auto i : facts()) {
        if (i->isZero())
            continue;
        auto jsv = i->toJson();
        if (jsv.isNull() || jsv.isUndefined())
            continue;
        jso.insert(i->name(), jsv);
    }
    if (jso.isEmpty())
        return {};
    return jso;
}
