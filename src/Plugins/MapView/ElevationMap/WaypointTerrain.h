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

#include "MissionTerrain.h"

#include <QFutureWatcher>
#include <QGeoPath>
#include <QList>
#include <QPointF>
#include <QPromise>

class Fact;
class Waypoint;

// Terrain analysis of one waypoint: its height above ground, the terrain profile
// of the path that leads to it, the collision check and the path correction.
//
// Everything a user sees of it in the mission is added here and removed in
// detach(): the "AGL" and "Path correction" rows of the waypoint menu, the
// widgets next to the altitude editors and the map highlight (the "alarm" and
// "pathColor" options of the waypoint, which the map reads).
class WaypointTerrain : public ItemTerrain
{
    Q_OBJECT
    Q_PROPERTY(bool collision READ collision NOTIFY collisionChanged)
    Q_PROPERTY(int unsafeAgl READ unsafeAgl CONSTANT)

public:
    explicit WaypointTerrain(Waypoint *wp);

    static WaypointTerrain *of(MissionItem *item);

    static constexpr int UNSAFE_AGL = 100;
    static constexpr double ALT_EPS = 0.1;

    // Height above ground, editable: altitude and AGL are two views of one value.
    // It has no data type on purpose: the mission stores and uploads typed fields
    // only, and AGL is derived from the elevation map, not a part of the mission.
    Fact *f_agl;
    Fact *f_correct; // "Path correction" row

    Waypoint *waypoint() const { return m_wp; }
    int agl() const;

    // terrain along the path to the waypoint: distance from the path start, elevation
    QList<QPointF> terrainProfile() const { return m_terrainProfile; }
    bool terrainProfileNeedUpdate() const;

    double minHeight() const { return m_minHeight; }
    double maxHeight() const { return m_maxHeight; }
    bool collision() const { return m_collision; }
    int unsafeAgl() const { return UNSAFE_AGL; }

    void detach() override;

public slots:
    void updateAgl();
    void sendTerrainProfileRequest();
    // the plugin broadcasts every profile it gets: take the one for this waypoint
    void buildTerrainProfile(const QGeoPath &path);
    void checkCollision();
    // reply: report the new points with responseCorrectPath() instead of inserting them
    void correctPath(bool reply = false);
    // the first waypoint ends the takeoff leg, which is not analysed
    void updateFirst();

private:
    struct TerrainInfo
    {
        QGeoPath terrainProfilePath;
        QList<QPointF> terrainProfile;
        double minHeight;
        double maxHeight;
    };

    Waypoint *m_wp;

    QTimer m_geoPathTimer;
    QList<QPointF> m_terrainProfile;
    QGeoPath m_terrainProfilePath;
    double m_terrainProfileMin{0};
    double m_terrainProfileMax{200};
    QFutureWatcher<TerrainInfo> m_watcher;

    double m_minHeight{0};
    double m_maxHeight{200};
    bool m_collision{false};

    bool m_reply{false};
    QFutureWatcher<QList<QGeoCoordinate>> m_pointsWatcher;

    bool m_aglComputing{false}; // AGL is being written here, not edited by the user

    Waypoint *prevWaypoint() const;
    MissionTerrain *missionTerrain() const;
    double startHmsl() const;

    void setTerrainProfile(const QList<QPointF> &v);
    void clearTerrainProfile();
    void setMinHeight(double v);
    void setMaxHeight(double v);
    void setCollision(bool v);
    void setComputedAgl(int v);

    static void createTerrainInfo(QPromise<TerrainInfo> &promise, const QGeoPath &path);
    static void getCorrectRoutePoints(QPromise<QList<QGeoCoordinate>> &promise,
                                      const QGeoPath &path,
                                      int hFirst,
                                      int hLast);

private slots:
    void calcAltitude();
    void recalcAltitude();
    void updateMinMaxHeight();
    void updateTerrainInfo();
    void updateAglEnabled();
    void updateMapAlarm();
    void insertNewPoints();
    void watchPrevious();

signals:
    void terrainProfileChanged();
    void collisionChanged();
    void minHeightChanged();
    void maxHeightChanged();

    void requestTerrainProfile(QGeoPath v);
    void responseCorrectPath(QList<QGeoCoordinate> v, int index);
};
