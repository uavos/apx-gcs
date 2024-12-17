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

    f_shot = new MissionField(this, "shot", tr("Shot"), tr("Make a cam shot on waypoint"), Enum);
    f_shot->setEnumStrings(QStringList() << "off"
                                         << "single"
                                         << "start"
                                         << "stop");
    f_dshot = new MissionField(this,
                               "dshot",
                               tr("Auto Shot"),
                               tr("Continuous cam shots distance"),
                               Int);
    f_dshot->setEnumStrings(QStringList() << "off");
    f_dshot->setUnits("m");
    f_dshot->setMin(0);

    // Add feets options
    m_isFeets = parent->isFeets();
    auto ft = std::round(f_speed->value().toInt() * parent->M2FT_COEF);
    f_speed->setOpt("editor", "EditorIntWithFeet.qml");
    f_speed->setOpt("ft", ft);
    ft = std::round(f_dshot->value().toInt() * parent->M2FT_COEF);
    f_dshot->setOpt("editor", "EditorIntWithFeet.qml");
    f_dshot->setOpt("ft", ft);
    connect(f_speed, &Fact::optsChanged, this, &WaypointActions::updateActionsValue);
    connect(f_dshot, &Fact::optsChanged, this, &WaypointActions::updateActionsValue);
    connect(parent, &Waypoint::isFeetsChanged, this, [this]() { setIsFeets(!m_isFeets); });
    connect(this, &WaypointActions::isFeetsChanged, this, &WaypointActions::updateActionsValue);
    // feets end

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
        // if (f->isZero())
        //     continue;
        // st.append(QString("%1=%2").arg(f->name()).arg(f->valueText()));

        if(!m_isFeets) {
            if (f->isZero())
                continue;
            st.append(QString("%1=%2").arg(f->name()).arg(f->valueText()));
        } else {
            if (!f->opts().contains("ft")) {
                if (f->isZero())
                    continue;
                st.append(QString("%1=%2").arg(f->name()).arg(f->valueText()));
            } else {
                if (f->opts().value("ft").toInt() == 0)
                    continue;
                st.append(QString("%1=%2").arg(f->name()).arg(f->opts().value("ft").toInt()));
            }
        }
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

QVariant WaypointActions::toVariant()
{
    QVariantMap m;
    for (auto i : facts()) {
        if (i->isZero())
            continue;
        QVariant v = i->toVariant();
        if (!v.isNull())
            m.insert(i->name(), v);
    }
    if (m.isEmpty())
        return {};
    return m;
}

bool WaypointActions::isFeets() const
{
    return m_isFeets;
}

void WaypointActions::setIsFeets(bool v)
{
    if (m_isFeets == v)
        return;
    m_isFeets = v;
    emit isFeetsChanged();
}
