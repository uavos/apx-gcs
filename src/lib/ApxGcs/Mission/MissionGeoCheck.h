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
#pragma once

#include <ApxMisc/DelayedEvent.h>
#include <QGeoCoordinate>
#include <QtCore>

class UnitMission;
class Geo;

// Checks the mission against the "deny" geofences: a waypoint inside a zone
// or a path between waypoints crossing a zone (the inside test matches the
// firmware Geofence module, altitude limits are not checked there either).
// Marks the waypoints with the options read by the map:
//  "denyZone" - the point is inside a zone or an adjacent path crosses one
//  "denyPath" - the path to the point crosses a zone
class MissionGeoCheck : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool denied READ denied NOTIFY deniedChanged)

public:
    explicit MissionGeoCheck(UnitMission *mission);

    bool denied() const { return m_denied; }

public slots:
    void schedule();
    void update();

private:
    struct Zone
    {
        Geo *geo;
        int shape;
        QGeoCoordinate ref; // origin of the local frame [m] x=east, y=north
        QList<QPointF> pts; // polygon vertices or line ends
        double radius;
    };

    UnitMission *m_mission;
    DelayedEvent m_update{50, false};
    bool m_denied{};

    QList<Zone> collectDenyZones();
    void watch(Geo *geo);
    static QPointF toLocal(const QGeoCoordinate &ref, const QGeoCoordinate &c);
    static bool pointInZone(const Zone &z, const QGeoCoordinate &c);
    static bool segmentInZone(const Zone &z, const QGeoCoordinate &a, const QGeoCoordinate &b);

signals:
    void deniedChanged();
};
