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
#include "MissionItem.h"
#include "MissionField.h"
#include "VehicleMission.h"

#include <App/AppRoot.h>
#include <QGeoCircle>

MissionItem::MissionItem(MissionGroup *parent,
                         const QString &name,
                         const QString &title,
                         const QString &descr)
    : Fact(parent, name, title, descr, Group | ModifiedGroup)
    , group(parent)
    , blockUpdateCoordinate(false)
    , m_bearing(0)
    , m_time(0)
    , m_distance(0)
    , m_totalDistance(0)
    , m_totalTime(0)
    , m_selected(false)
    , m_isFeet(false)
{
    setOpt("pos", QPointF(0.25, 0.5));

    f_order = new MissionField(this, "order", tr("Order"), tr("Object sequence number"), Int);
    f_order->setMin(1);
    updateOrderState();
    connect(f_order, &Fact::valueChanged, this, &MissionItem::updateOrder);
    connect(this, &Fact::numChanged, this, &MissionItem::updateOrderState);
    connect(group, &Fact::sizeChanged, this, &MissionItem::updateOrderState);

    f_latitude = new MissionField(this,
                                  "lat",
                                  tr("Latitude"),
                                  tr("Global position latitude"),
                                  Float);
    f_latitude->setUnits("lat");
    f_longitude = new MissionField(this,
                                   "lon",
                                   tr("Longitude"),
                                   tr("Global position longitude"),
                                   Float);
    f_longitude->setUnits("lon");

    // This is a temporary solution until the feet-meters conversion of gcs is completed.
    f_feet = new Fact(this,
                      "ft_m",
                      tr("ft"),
                      tr("switch between feet and meters"),
                      Action,"swap-vertical");
    f_feet->setOpt("alignleft", true);
    f_feet->setVisible(false);

    connect(f_feet, &Fact::triggered, this, &MissionItem::changeFeetMeters);

    f_remove = new Fact(this, "remove", tr("Remove"), tr("Remove mission item"), Action | Remove);
    connect(f_remove, &Fact::triggered, this, &Fact::deleteFact);

    //active index
    if (group->f_activeIndex) {
        Fact *f_select = new Fact(this,
                                  "select",
                                  tr("Set as current"),
                                  group->f_activeIndex->descr(),
                                  Action | Apply,
                                  "target");
        connect(f_select, &Fact::triggered, this, &MissionItem::selectTriggered);

        connect(group->f_activeIndex, &Fact::valueChanged, this, [this]() {
            setActive(group->f_activeIndex->value().toInt() == num());
        });
        setActive(group->f_activeIndex->value().toInt() == num());
    }

    connect(f_latitude, &Fact::valueChanged, this, &MissionItem::updateCoordinate);
    connect(f_longitude, &Fact::valueChanged, this, &MissionItem::updateCoordinate);

    connect(this, &MissionItem::coordinateChanged, this, &MissionItem::updatePath);

    connect(this, &Fact::numChanged, this, &MissionItem::updatePath, Qt::QueuedConnection);

    //selection support
    connect(group->mission,
            &VehicleMission::selectedItemChanged,
            this,
            &MissionItem::updateSelected);

    //title
    connect(this, &Fact::numChanged, this, &MissionItem::updateTitle);
    updateTitle();

    //totals
    connect(this, &MissionItem::timeChanged, group, &MissionGroup::updateTime);
    connect(this, &MissionItem::distanceChanged, group, &MissionGroup::updateDistance);

    connect(this, &MissionItem::totalTimeChanged, this, &MissionItem::updateStatus);
    connect(this, &MissionItem::totalDistanceChanged, this, &MissionItem::updateStatus);
    updateStatus();
}

void MissionItem::hashData(QCryptographicHash *h) const
{
    for (int i = 0; i < size(); ++i) {
        child(i)->hashData(h);
    }
}

QVariant MissionItem::toVariant()
{
    QVariant v = Fact::toVariant();
    if (v.isNull())
        return {};
    auto h = v.value<QVariantMap>();
    h.remove(f_order->name());
    return h;
}

int MissionItem::missionItemType() const
{
    return group->missionItemType();
}

void MissionItem::updateTitle()
{
    setTitle(QString::number(num() + 1));
}

void MissionItem::updateStatus()
{
    uint d = totalDistance();
    uint t = totalTime();
    if ((d | t) == 0)
        setValue(QVariant());
    else {
        QStringList st;
        st.append(AppRoot::distanceToString(d));
        st.append(AppRoot::timeToString(t, false));
        setValue(st.join(' '));
    }
}

void MissionItem::updateCoordinate()
{
    if (blockUpdateCoordinate)
        return;
    setCoordinate(QGeoCoordinate(f_latitude->value().toDouble(), f_longitude->value().toDouble()));
}

void MissionItem::updatePath()
{
    setGeoPath(getPath());
    //check to propagate updates to next items
    MissionItem *next = nextItem();
    if (next) {
        QGeoCoordinate p1, p2;
        if (m_geoPath.path().size() >= 2)
            p1 = QGeoCoordinate(m_geoPath.path().last());
        if (next->geoPath().path().size() >= 2)
            p2 = QGeoCoordinate(next->geoPath().path().first());
        if (next->geoPath().path().size() < 2 || m_geoPath.path().size() < 2
            || p1.latitude() != p2.latitude() || p1.longitude() != p2.longitude()) {
            next->updatePath();
        }
    }
}

void MissionItem::updateOrder()
{
    int n = f_order->value().toInt() - 1;
    if (n < 0)
        n = 0;
    else if (n >= group->size())
        n = group->size() - 1;
    if (n == num() || group->size() < 2) {
        f_order->setValue(num() + 1);
        return;
    }
    move(n, true);
}
void MissionItem::updateOrderState()
{
    f_order->setValue(num() + 1);
    f_order->setEnabled(group->size() > 1);
}
void MissionItem::updateSelected()
{
    setSelected(group->mission->selectedItem() == this);
}

void MissionItem::selectTriggered()
{
    group->f_activeIndex->setValue(num());
}

void MissionItem::resetPath()
{
    m_geoPath = QGeoPath();
}

QGeoPath MissionItem::getPath()
{
    return QGeoPath();
}

MissionItem *MissionItem::prevItem() const
{
    return static_cast<MissionItem *>(parentFact()->child(indexInParent() - 1));
}
MissionItem *MissionItem::nextItem() const
{
    return static_cast<MissionItem *>(parentFact()->child(indexInParent() + 1));
}

QGeoRectangle MissionItem::boundingGeoRectangle() const
{
    QGeoRectangle r(QGeoCircle(coordinate(), 20).boundingGeoRectangle());
    r.setWidth(r.width() * 1.2);
    r.setHeight(r.height() * 1.2);
    return r;
}

QGeoCoordinate MissionItem::coordinate() const
{
    return m_coordinate;
}
void MissionItem::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
    blockUpdateCoordinate = true;
    f_latitude->setValue(v.latitude());
    f_longitude->setValue(v.longitude());
    blockUpdateCoordinate = false;
}
QGeoPath MissionItem::geoPath() const
{
    return m_geoPath;
}
void MissionItem::setGeoPath(const QGeoPath &v)
{
    if (m_geoPath == v)
        return;
    m_geoPath = v;
    emit geoPathChanged();
}
double MissionItem::bearing() const
{
    return m_bearing;
}
void MissionItem::setBearing(const double &v)
{
    if (m_bearing == v)
        return;
    m_bearing = v;
    emit bearingChanged();
}
uint MissionItem::time() const
{
    return m_time;
}
void MissionItem::setTime(uint v)
{
    if (m_time == v)
        return;
    m_time = v;
    emit timeChanged();
}
uint MissionItem::distance() const
{
    return m_distance;
}
void MissionItem::setDistance(uint v)
{
    if (m_distance == v)
        return;
    m_distance = v;
    emit distanceChanged();
}
uint MissionItem::totalDistance() const
{
    return m_totalDistance;
}
void MissionItem::setTotalDistance(uint v)
{
    if (m_totalDistance == v)
        return;
    m_totalDistance = v;
    emit totalDistanceChanged();
}
uint MissionItem::totalTime() const
{
    return m_totalTime;
}
void MissionItem::setTotalTime(uint v)
{
    if (m_totalTime == v)
        return;
    m_totalTime = v;
    emit totalTimeChanged();
}
bool MissionItem::selected() const
{
    return m_selected;
}
void MissionItem::setSelected(bool v)
{
    if (m_selected == v)
        return;
    m_selected = v;
    emit selectedChanged();
    if (v)
        group->mission->setSelectedItem(this);
    else if (group->mission->selectedItem() == this)
        group->mission->setSelectedItem(nullptr);
}

bool MissionItem::isFeet() const
{
    return m_isFeet;
}

void MissionItem::setIsFeet(bool v)
{
    if (m_isFeet == v)
        return;
    m_isFeet = v;
    emit isFeetChanged();
}

void MissionItem::changeFeetMeters()
{
    setIsFeet(!m_isFeet);
    if(m_isFeet)
        f_feet->setTitle("m");
    else
        f_feet->setTitle("ft");
}
