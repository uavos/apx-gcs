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

#include "ElevationDB.h"
#include <Fact/Fact.h>

#include <memory>

#include <QQmlComponent>
#include <QGeoCoordinate>
#include <QtCore>
#include <QMap>
#include <QSet>

#include <Mission/MissionItem.h>

// ===== Analyze elevation route =====
#include <QGeoPath>
#include <QPointF>
// ===================================

class Unit;
class UnitMission;
class MissionTools;

class ElevationMap : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QGeoPath geoPath READ geoPath WRITE setGeoPath NOTIFY geoPathChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(double elevation READ elevation WRITE setElevation NOTIFY elevationChanged)

public:
    explicit ElevationMap(Fact *parent = nullptr);

    Fact *f_use;
    Fact *f_path;
    Fact *f_util;
    Fact *f_control{nullptr};
    Fact *f_refStatus{nullptr};
    Fact *f_refHmsl{nullptr};

    Unit *unit() const;
    UnitMission *mission() const;
    MissionTools *missionTools() const;
    Fact *aglset() const;

    Q_INVOKABLE void setElevationByCoordinate(const QGeoCoordinate &coordinate);
    void setCoordinateWithElevation(const QGeoCoordinate &coordinate);
    void setTerrainProfile(const QGeoPath &path);

    QGeoPath geoPath() const;
    void setGeoPath(const QGeoPath &v);

    QGeoCoordinate coordinate() const;
    void setCoordinate(const QGeoCoordinate &coordinate);
    double elevation() const;
    void setElevation(double v);

    // ===== Analyze elevation route =====
    // static constexpr int TERRAIN_STEP = 10;
    static constexpr int TERRAIN_STEP = 15;

protected:
    QGeoPath m_geoPath;
    QGeoCoordinate m_coordinate;
    double m_elevation;

private:
    QSharedPointer<AbstractElevationDB> m_elevationDB;
    QMap<QString, int> m_waypoints;
    QSet<QString> m_runways;
    QSet<QString> m_pois;

    void createElevationDatabase();
    void setMissionValues(bool b);
    void setWaypointsValues(bool b);
    void setRunwaysValues(bool b);
    void setPoisValues(bool b);
    void clearMissionPoints();
    QObject *qml;

private slots:
    double getRefPointHmsl();
    void onOpenTriggered();
    void updateMission();
    void updateRefPoint();
    void setMissionAgl();
    void correctUnsafePaths();
    void getPluginEnableControl();
    void changeExternalsVisibility();
    void setStartPointElevation();
    void updateDBUtility();

signals:
    void coordinateChanged(QGeoCoordinate coordinate);
    void geoPathChanged(QGeoPath geoPath);
    void elevationChanged();
};
