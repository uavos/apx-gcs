/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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

#include <QtCore>
#include <QtPositioning>

// one point of the mission route with the wind at its planned altitude
struct RoutePoint
{
    QGeoCoordinate coordinate;
    int leg{0};          // destination waypoint index (0 based)
    bool waypoint{false}; // the point is the waypoint itself
    double dist{0};      // from the route start, m
    double track{0};     // course of the route on the point, deg
    double altMsl{0};    // planned altitude, m MSL
    double altAgl{0};    // planned altitude above the mission reference (home), m
    double windSpeed{qQNaN()}; // at the planned altitude, m/s
    double windDir{qQNaN()};   // wind from, deg
    QString time;        // forecast hour used (UTC)
};

class WeatherModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        CoordinateRole = Qt::UserRole + 1,
        LegRole,
        WaypointRole,
        DistRole,
        TrackRole,
        AltMslRole,
        AltAglRole,
        WindSpeedRole,
        WindDirRole,
        TimeRole,
    };

    explicit WeatherModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // all route points; the model shows every stride-th point and the waypoints,
    // at most shown_max rows (the stride is raised when the route is long)
    void reset(const QList<RoutePoint> &list);
    void setStride(int stride);
    int stride() const { return _stride; }
    int allCount() const { return _all.size(); }

private:
    static constexpr int shown_max = 800;
    QList<RoutePoint> _all;
    QList<RoutePoint> _items; // shown
    int _stride{1};
    void rebuild();
};
