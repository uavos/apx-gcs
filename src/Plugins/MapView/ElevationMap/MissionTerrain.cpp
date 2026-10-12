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
#include "MissionTerrain.h"
#include "WaypointTerrain.h"

#include <Fact/Fact.h>
#include <Mission/MissionItem.h>
#include <Mission/UnitMission.h>
#include <Mission/Waypoint.h>

#include <cfloat>

MissionTerrain::MissionTerrain(UnitMission *mission)
    : QObject(mission)
    , m_mission(mission)
{
    connect(this, &MissionTerrain::startElevationChanged, this, &MissionTerrain::updateMinHeight);
    connect(this, &MissionTerrain::startElevationChanged, this, &MissionTerrain::updateMaxHeight);
    connect(mission, &UnitMission::emptyChanged, this, &MissionTerrain::setDefaultMinMaxHeight);
}

MissionTerrain *MissionTerrain::of(UnitMission *mission)
{
    if (!mission)
        return nullptr;
    // (Fact hides QObject::findChild with its own lookup by path)
    auto t = mission->QObject::findChild<MissionTerrain *>(QString(), Qt::FindDirectChildrenOnly);
    return t ? t : new MissionTerrain(mission);
}

void MissionTerrain::setStartElevation(double v)
{
    if (m_startElevation == v)
        return;
    m_startElevation = v;
    emit startElevationChanged();
}

void MissionTerrain::setMinHeight(double v)
{
    if (m_minHeight == v)
        return;
    m_minHeight = v;
    emit minHeightChanged();
}

void MissionTerrain::setMaxHeight(double v)
{
    if (m_maxHeight == v)
        return;
    m_maxHeight = v;
    emit maxHeightChanged();
}

void MissionTerrain::setCollision(bool v)
{
    if (m_collision == v)
        return;
    m_collision = v;
    emit collisionChanged();
}

void MissionTerrain::checkCollision()
{
    auto wps = m_mission->f_wp;
    for (int i = 0; i < wps->size(); ++i) {
        auto t = WaypointTerrain::of(static_cast<MissionItem *>(wps->child(i)));
        if (t && t->collision()) {
            setCollision(true);
            return;
        }
    }
    setCollision(false);
}

void MissionTerrain::updateMinHeight()
{
    if (m_mission->missionSize() <= 0)
        return;
    double min{0};
    auto wps = m_mission->f_wp;
    for (int i = 0; i < wps->size(); ++i) {
        auto t = WaypointTerrain::of(static_cast<MissionItem *>(wps->child(i)));
        if (t)
            min = qMin(min, t->minHeight());
    }
    setMinHeight(qMin(min, m_startElevation));
}

void MissionTerrain::updateMaxHeight()
{
    if (m_mission->missionSize() <= 0)
        return;
    double max{0};
    auto wps = m_mission->f_wp;
    for (int i = 0; i < wps->size(); ++i) {
        auto t = WaypointTerrain::of(static_cast<MissionItem *>(wps->child(i)));
        if (t)
            max = qMax(max, t->maxHeight());
    }
    setMaxHeight(qMax(max, m_startElevation));
}

void MissionTerrain::setDefaultMinMaxHeight()
{
    if (!m_mission->empty())
        return;
    setMinHeight(0);
    setMaxHeight(200);
}

ItemTerrain::ItemTerrain(MissionItem *item)
    : QObject(item)
    , m_item(item)
{
    // request the terrain elevation shortly after the item was moved
    m_timer.setSingleShot(true);
    m_timer.setInterval(TIMEOUT);
    connect(item, &MissionItem::coordinateChanged, this, [this]() {
        if (!m_timer.isActive())
            m_timer.start();
    });
    connect(&m_timer, &QTimer::timeout, this, &ItemTerrain::sendElevationRequest);
}

ItemTerrain *ItemTerrain::of(MissionItem *item)
{
    if (!item)
        return nullptr;
    return item->QObject::findChild<ItemTerrain *>(QString(), Qt::FindDirectChildrenOnly);
}

void ItemTerrain::setInfoField(Fact *f)
{
    m_infoField = f;
    f->setOpt("extrainfo", "qrc:/ExtraInfoElevation.qml");
}

void ItemTerrain::detach()
{
    if (m_infoField)
        m_infoField->setOpt("extrainfo", QVariant());
}

void ItemTerrain::setElevation(double v)
{
    if (m_elevation == v || (std::isnan(m_elevation) && std::isnan(v)))
        return;
    m_elevation = v;
    emit elevationChanged();
}

void ItemTerrain::extractElevation(const QGeoCoordinate &coordinate)
{
    const auto c = m_item->coordinate();
    auto latDiff = std::abs(c.latitude() - coordinate.latitude());
    auto lonDiff = std::abs(c.longitude() - coordinate.longitude());
    if (latDiff > DBL_EPSILON || lonDiff > DBL_EPSILON)
        return;
    setElevation(coordinate.altitude());
}

void ItemTerrain::sendElevationRequest()
{
    emit requestElevation(m_item->coordinate());
}
