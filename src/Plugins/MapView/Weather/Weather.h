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

#include "WeatherModel.h"

#include <Fact/Fact.h>
#include <QtCore>
#include <QtNetwork>
#include <QtPositioning>

class Unit;

// Open-Meteo hourly wind profile at one point of the route
struct ForecastNode
{
    struct Level
    {
        double h; // height, m MSL
        double u; // wind east component, m/s (blowing to)
        double v; // wind north component, m/s
    };
    QGeoCoordinate coordinate;
    double elevation{0};         // model terrain, m MSL
    QList<qint64> hours;         // epoch seconds (UTC) of the hourly values
    QList<QList<Level>> profile; // [hour] levels sorted by height

    int hourIndex(qint64 epoch) const;
    void windAt(int hour, double altMsl, double *u, double *v) const;
};

// Wind along the mission route at the planned altitude, every 100 m and on the waypoints
class Weather : public Fact
{
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel *points READ points CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // row of the route point with the info panel pinned on the map (click), -1 is none
    Q_PROPERTY(int selected READ selected WRITE setSelected NOTIFY selectedChanged)
    // the map shows every stride-th point (set from the map zoom), the waypoints always
    Q_PROPERTY(int stride READ stride WRITE setStride NOTIFY strideChanged)

public:
    explicit Weather(Fact *parent = nullptr);

    Fact *f_enabled;

    QAbstractListModel *points() const { return _points; }
    QString status() const { return _status; }
    int selected() const { return _selected; }
    void setSelected(int row);
    int stride() const;
    void setStride(int v);

private:
    // one mission leg: the path to the waypoint and its altitude
    struct Leg
    {
        QGeoPath path;
        double alt{0}; // waypoint altitude
        bool amsl{false};
        bool atrack{false}; // linear altitude control along the leg
    };

    WeatherModel *_points;

    QNetworkAccessManager _net;
    QPointer<QNetworkReply> _reply;
    QTimer _rebuildTimer; // debounce of the mission edits
    QTimer _refreshTimer; // forecast period

    QPointer<Unit> _unit;
    QObject *_watcher{nullptr}; // context of the mission connections, deleted on rebind

    QList<Leg> _legs;
    double _refHmsl{0}; // mission reference altitude (home), m MSL
    bool _refValid{false};

    QList<ForecastNode> _nodes;
    QList<QGeoCoordinate> _requested; // nodes of the request in progress
    qint64 _forecastTimeMs{0};
    qint64 _retryMs{0};

    int _selected{-1};
    QString _status;
    QString _error;
    void setStatus(const QString &status);

    Fact *refFact() const;
    void bindUnit();
    void scheduleRebuild();
    bool buildRoute();
    QList<QGeoCoordinate> forecastNodes() const;
    bool nodesCover(const QList<QGeoCoordinate> &nodes) const;
    void requestForecast(const QList<QGeoCoordinate> &nodes);
    void compute();
    void clear();

    // wind at the point: distance weighted of the two nearest nodes
    bool windAt(const QGeoCoordinate &c, double altMsl, qint64 epoch, double *u, double *v, QString *time) const;

private slots:
    void onEnabledChanged();
    void rebuild();
    void onReply();
    void refresh();

signals:
    void statusChanged();
    void selectedChanged();
    void strideChanged();
};
