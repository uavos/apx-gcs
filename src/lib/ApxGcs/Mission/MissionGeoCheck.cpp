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
#include "MissionGeoCheck.h"

#include "Geo.h"
#include "UnitMission.h"
#include "Waypoint.h"

#include <algorithm>
#include <cmath>
#include <QLineF>

namespace {

constexpr double kLineZoneExtent = 1.0e+5; // LINE zone depth behind the line [m]
constexpr double kMinLength = 1.0;         // shorter lines are ignored [m]

bool crosses(const QLineF &a, const QLineF &b)
{
    return a.intersects(b) == QLineF::BoundedIntersection;
}

// side ray of a LINE zone from the point p: the firmware treats the left side
// of the line (p1 -> p2) as inside, QLineF::normalVector points to the right
QLineF sideRay(const QLineF &line, const QPointF &p)
{
    const QLineF n = line.normalVector().unitVector();
    return QLineF(p, p + QPointF(-n.dx(), -n.dy()) * kLineZoneExtent);
}

} // namespace

MissionGeoCheck::MissionGeoCheck(UnitMission *mission)
    : QObject(mission)
    , m_mission(mission)
{
    connect(&m_update, &DelayedEvent::triggered, this, &MissionGeoCheck::update);
    for (Fact *group : {static_cast<Fact *>(mission->f_wp), static_cast<Fact *>(mission->f_geo)}) {
        connect(group, &Fact::sizeChanged, this, &MissionGeoCheck::schedule);
        connect(group, &Fact::itemMoved, this, &MissionGeoCheck::schedule);
    }
}

void MissionGeoCheck::schedule()
{
    m_update.schedule();
}

// flat local frame around the zone: accurate enough for mission scale distances
QPointF MissionGeoCheck::toLocal(const QGeoCoordinate &ref, const QGeoCoordinate &c)
{
    const double d = ref.distanceTo(c);
    const double a = qDegreesToRadians(ref.azimuthTo(c));
    return QPointF(d * std::sin(a), d * std::cos(a));
}

// item signals are connected on every scan and not on insertion:
// the insertion signals come from the base class constructor
void MissionGeoCheck::watch(Geo *geo)
{
    const auto c = Qt::UniqueConnection;
    connect(geo, &MissionItem::coordinateChanged, this, &MissionGeoCheck::schedule, c);
    connect(geo, &Geo::polygonChanged, this, &MissionGeoCheck::schedule, c);
    connect(geo->f_p2, &MissionPoint::coordinateChanged, this, &MissionGeoCheck::schedule, c);
    for (auto f : {geo->f_role, geo->f_shape, geo->f_radius})
        connect(f, &Fact::valueChanged, this, &MissionGeoCheck::schedule, c);
}

QList<MissionGeoCheck::Zone> MissionGeoCheck::collectDenyZones()
{
    QList<Zone> zones;
    auto group = m_mission->f_geo;
    for (int i = 0; i < group->size(); ++i) {
        auto geo = qobject_cast<Geo *>(group->child(i));
        if (!geo)
            continue;
        watch(geo);

        if (geo->f_role->value().toInt() != xbus::mission::geo_s::DENY)
            continue;

        Zone z{geo, geo->f_shape->value().toInt(), geo->coordinate(), {}, 0};
        if (!z.ref.isValid())
            continue;
        switch (z.shape) {
        case xbus::mission::geo_s::CIRCLE:
            z.radius = std::abs(geo->f_radius->value().toDouble());
            break;
        case xbus::mission::geo_s::POLYGON:
            for (const auto &p : geo->polygon().perimeter())
                z.pts.append(toLocal(z.ref, p));
            if (z.pts.size() < 3)
                continue;
            break;
        case xbus::mission::geo_s::LINE: {
            const auto p2 = geo->f_p2->coordinate();
            if (!p2.isValid() || z.ref.distanceTo(p2) < kMinLength)
                continue;
            z.pts = {QPointF(0, 0), toLocal(z.ref, p2)};
        } break;
        default:
            continue;
        }
        zones.append(z);
    }
    return zones;
}

bool MissionGeoCheck::pointInZone(const Zone &z, const QGeoCoordinate &c)
{
    switch (z.shape) {
    case xbus::mission::geo_s::CIRCLE:
        return z.ref.distanceTo(c) <= z.radius;
    case xbus::mission::geo_s::POLYGON:
        return z.geo->polygon().contains(c);
    case xbus::mission::geo_s::LINE: {
        // within the segment bounds, on the left side of the line
        const QLineF line(z.pts.at(0), z.pts.at(1));
        const QPointF p = toLocal(z.ref, c) - line.p1();
        const QPointF d(line.dx(), line.dy());
        const double t = QPointF::dotProduct(p, d) / QPointF::dotProduct(d, d);
        return t >= 0 && t <= 1 && (d.x() * p.y() - d.y() * p.x()) >= 0;
    }
    }
    return false;
}

bool MissionGeoCheck::segmentInZone(const Zone &z, const QGeoCoordinate &a, const QGeoCoordinate &b)
{
    if (pointInZone(z, a) || pointInZone(z, b))
        return true;

    const QLineF s(toLocal(z.ref, a), toLocal(z.ref, b));
    if (s.length() < kMinLength)
        return false; // degenerate segment, the ends are checked above

    switch (z.shape) {
    case xbus::mission::geo_s::CIRCLE: {
        // distance from the zone center (local origin) to the segment
        const QPointF d(s.dx(), s.dy());
        const double t = std::clamp(-QPointF::dotProduct(s.p1(), d) / QPointF::dotProduct(d, d),
                                    0.0,
                                    1.0);
        return QLineF(QPointF(0, 0), s.pointAt(t)).length() <= z.radius;
    }
    case xbus::mission::geo_s::POLYGON:
        for (int i = 0, j = z.pts.size() - 1; i < z.pts.size(); j = i++) {
            if (crosses(s, QLineF(z.pts.at(j), z.pts.at(i))))
                return true;
        }
        return false;
    case xbus::mission::geo_s::LINE: {
        const QLineF line(z.pts.at(0), z.pts.at(1));
        const auto crossesSide = [&s, &line](const QPointF &p) {
            QLineF ray = sideRay(line, p);
            ray.setLength(std::max(QLineF(p, s.p1()).length(), QLineF(p, s.p2()).length()));
            return crosses(s, ray);
        };
        return crosses(s, line) || crossesSide(line.p1()) || crossesSide(line.p2());
    }
    }
    return false;
}

void MissionGeoCheck::update()
{
    const auto zones = collectDenyZones();
    auto group = m_mission->f_wp;
    const int cnt = group->size();

    QList<Waypoint *> wps;
    QList<bool> point(cnt, false), path(cnt, false);

    for (int i = 0; i < cnt; ++i) {
        auto wp = qobject_cast<Waypoint *>(group->child(i));
        wps.append(wp);
        if (!wp)
            continue;
        connect(wp,
                &MissionItem::geoPathChanged,
                this,
                &MissionGeoCheck::schedule,
                Qt::UniqueConnection);
        connect(wp,
                &MissionItem::coordinateChanged,
                this,
                &MissionGeoCheck::schedule,
                Qt::UniqueConnection);

        const auto p = wp->geoPath().path();
        for (const auto &z : zones) {
            if (!point[i])
                point[i] = pointInZone(z, wp->coordinate());
            for (int k = 1; k < p.size() && !path[i]; ++k)
                path[i] = segmentInZone(z, p.at(k - 1), p.at(k));
        }
    }

    bool denied = false;
    for (int i = 0; i < cnt; ++i) {
        auto wp = wps.at(i);
        if (!wp)
            continue;
        // the point and both ends of a denied path are marked
        const bool zone = point[i] || path[i] || ((i + 1) < cnt && path[i + 1]);
        denied |= zone;
        const QVariant vZone = zone ? QVariant(true) : QVariant();
        const QVariant vPath = path[i] ? QVariant(true) : QVariant();
        if (wp->opts().value("denyZone") != vZone)
            wp->setOpt("denyZone", vZone);
        if (wp->opts().value("denyPath") != vPath)
            wp->setOpt("denyPath", vPath);
    }

    if (m_denied == denied)
        return;
    m_denied = denied;
    emit deniedChanged();
}
