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
#include "MissionGroup.h"
#include "MissionItem.h"
#include "MissionMapItemsModel.h"
#include "UnitMission.h"

#include <App/AppRoot.h>

MissionGroup::MissionGroup(UnitMission *parent,
                           const QString &name,
                           const QString &title,
                           const QString &descr,
                           Fact *activeIndex)
    : Fact(parent, name, title, descr, Group | ModifiedGroup | Count)
    , mission(parent)
    , f_activeIndex(activeIndex)
    , _descr(descr)
    , m_distance(0)
    , m_time(0)
{
    m_mapModel = new MissionMapItemsModel(this);

    //setSection(tr("Mission elements"));
    mission->groups.append(this);

    f_clear = new Fact(this,
                       "clear",
                       tr("Clear"),
                       tr("Remove all objects"),
                       Action | Remove | CloseOnTrigger);
    f_clear->setEnabled(false);
    connect(f_clear, &Fact::triggered, this, &MissionGroup::clearGroup);

    connect(this, &Fact::sizeChanged, this, &MissionGroup::updateStatus);

    connect(this, &Fact::sizeChanged, this, [this]() { setModified(true); });

    //time & distance
    updateTimeTimer.setSingleShot(true);
    updateTimeTimer.setInterval(1000);
    connect(&updateTimeTimer, &QTimer::timeout, this, &MissionGroup::updateTimeDo);
    updateDistanceTimer.setSingleShot(true);
    updateDistanceTimer.setInterval(1000);
    connect(&updateDistanceTimer, &QTimer::timeout, this, &MissionGroup::updateDistanceDo);

    connect(this, &Fact::sizeChanged, this, &MissionGroup::updateTime);
    connect(this, &Fact::sizeChanged, this, &MissionGroup::updateDistance);

    //descr
    connect(this, &MissionGroup::distanceChanged, this, &MissionGroup::updateDescr);
    connect(this, &MissionGroup::timeChanged, this, &MissionGroup::updateDescr);

    updateStatus();
    updateDescr();
}

void MissionGroup::updateStatus()
{
    int sz = size();
    if (sz > 0)
        setValue(QString("[%1]").arg(sz));
    else
        setValue(QVariant());
    f_clear->setEnabled(sz > 0);
}

void MissionGroup::updateDescr()
{
    uint d = distance();
    uint t = time();
    if ((d | t) == 0 && (!_descr.isEmpty()))
        setDescr(_descr);
    else {
        QStringList st;
        st.append(AppRoot::distanceToString(d));
        st.append(AppRoot::timeToString(t, false));
        setDescr(st.join(' '));
    }
}

void MissionGroup::updateTime()
{
    updateTimeTimer.start();
}
void MissionGroup::updateTimeDo()
{
    uint v = 0;
    for (auto i : facts()) {
        MissionItem *wp = static_cast<MissionItem *>(i);
        v += wp->time();
        wp->setTotalTime(v);
    }
    setTime(v);
}
void MissionGroup::updateDistance()
{
    updateDistanceTimer.start();
}
void MissionGroup::updateDistanceDo()
{
    uint v = 0;
    for (auto i : facts()) {
        MissionItem *wp = static_cast<MissionItem *>(i);
        v += wp->distance();
        wp->setTotalDistance(v);
    }
    setDistance(v);
}

uint MissionGroup::distance() const
{
    if (m_distance == 0)
        const_cast<MissionGroup *>(this)->updateDistanceDo();
    return m_distance;
}
void MissionGroup::setDistance(uint v)
{
    if (m_distance == v)
        return;
    m_distance = v;
    emit distanceChanged();
}
uint MissionGroup::time() const
{
    if (m_time == 0)
        const_cast<MissionGroup *>(this)->updateTimeDo();
    return m_time;
}
void MissionGroup::setTime(uint v)
{
    if (m_time == v)
        return;
    m_time = v;
    emit timeChanged();
}
QAbstractListModel *MissionGroup::mapModel() const
{
    return m_mapModel;
}

void MissionGroup::add(const QGeoCoordinate &p)
{
    if (!p.isValid()) {
        return;
    }
    addObject(p);
}

MissionItem *MissionGroup::addObject(const QGeoCoordinate &p)
{
    MissionItem *f = createObject();
    f->backup();
    f->f_latitude->setValue(p.latitude());
    f->f_longitude->setValue(p.longitude());
    return f;
}

void MissionGroup::clearGroup()
{
    deleteChildren();
    setDistance(0);
    setTime(0);
    setModified(true);
}

void MissionGroup::fromJson(const QJsonValue &jsv)
{
    clearGroup();
    for (const auto &i : jsv.toArray()) {
        createObject()->fromJson(i);
    }
}
