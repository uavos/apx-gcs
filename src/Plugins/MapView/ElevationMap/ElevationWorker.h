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

#include <QGeoCoordinate>
#include <QGeoPath>
#include <QHash>
#include <QMutex>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QWaitCondition>

#include <atomic>
#include <deque>
#include <memory>

class ElevationTile;

/**
 * Single worker thread that owns the ASTER tiles and serves all requests.
 *
 * - all tile access happens in this thread only (no locking around tile data);
 * - point requests are served before terrain profiles;
 * - a new profile request for the same path endpoints replaces the pending one
 *   and cancels the running one, so dragging a waypoint never queues stale work;
 * - a new "elevation" (map hover) request replaces the pending one.
 */
class ElevationWorker : public QThread
{
    Q_OBJECT

public:
    static constexpr int MAX_TILES = 4;               // open tiles kept (LRU)
    static constexpr double DEFAULT_SAMPLE_STEP = 30; // used when the tile is unknown, m

    explicit ElevationWorker(const QString &dbPath, QObject *parent = nullptr);
    ~ElevationWorker() override;

    void requestElevation(double lat, double lon);
    void requestCoordinate(double lat, double lon);
    void requestTerrainProfile(const QGeoPath &path);
    void requestAreaMax(double lat, double lon, double radius);
    void setCorridor(double meters);
    void stop();

signals:
    void elevationReady(double elevation);
    void coordinateReady(QGeoCoordinate coordinate);
    void terrainProfileReady(QGeoPath path);
    void terrainProfileCenterReady(QGeoPath path, QList<double> centerElevations);
    void areaMaxReady(double elevation);

protected:
    void run() override;

private:
    struct Job
    {
        enum Type { Elevation, Coordinate, AreaMax, Profile };
        Type type;
        double lat{0};
        double lon{0};
        double radius{0};
        QGeoPath path;
        QGeoCoordinate first; // profile endpoints (altitude stripped) used to coalesce
        QGeoCoordinate last;
    };

    static Job makeProfileJob(const QGeoPath &path);
    static bool sameEndpoints(const Job &a, const Job &b);

    void wake();
    bool takeJob(Job &job);
    void processPoint(const Job &job);
    void processProfile(const Job &job);
    void processAreaMax(const Job &job);
    ElevationTile *tile(double lat, double lon);
    double elevationAt(double lat, double lon);
    // sampling step across the corridor: the map resolution at the point, m
    double sampleStep(const QGeoCoordinate &p);
    // highest terrain across the corridor and the terrain under the point itself;
    // false when there is no data at the point
    bool corridorRange(
        const QGeoCoordinate &p, double azimuth, double halfWidth, double &max, double &center);
    bool circleRange(const QGeoCoordinate &p, double radius, double &max, double &center);
    // highest terrain inside the circle over the samples that have data, NaN when none
    double areaMax(const QGeoCoordinate &p, double radius);

    QString m_dbPath;

    QMutex m_mutex;
    QWaitCondition m_wait;
    std::deque<Job> m_points;
    std::deque<Job> m_profiles;
    bool m_hasCurrent{false};
    Job m_current;
    std::atomic<bool> m_cancelCurrent{false};
    std::atomic<bool> m_quit{false};
    std::atomic<int> m_corridor{100}; // m

    // last used tile (most lookups of a profile hit the same tile)
    ElevationTile *m_lastTile{nullptr};
    int m_lastTileLat{1000};
    int m_lastTileLon{1000};

    // worker thread only
    QHash<QString, std::shared_ptr<ElevationTile>> m_tiles;
    QStringList m_tilesLru;
    QSet<QString> m_missing;
};
