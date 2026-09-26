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

#include <QGeoCoordinate>
#include <QGeoPath>
#include <QMap>
#include <QPointF>
#include <QQmlComponent>
#include <QSet>
#include <QtCore>

#include <Mission/MissionItem.h>

class Unit;
class UnitMission;
class MissionTools;

class ElevationMap : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QGeoPath geoPath READ geoPath WRITE setGeoPath NOTIFY geoPathChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(double elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    // elevation files found in the configured directory
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    // the elevation files cover the mission area (at least one item is on a file)
    Q_PROPERTY(bool covered READ covered NOTIFY coveredChanged)
    // plugin enabled, in use, files available and covering the mission:
    // when false the plugin is fully passive (no fields, icons or alarms)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    // current unit: highest terrain within the corridor radius and height above it (NaN when unknown)
    Q_PROPERTY(double unitTerrain READ unitTerrain NOTIFY unitAglChanged)
    Q_PROPERTY(double unitAgl READ unitAgl NOTIFY unitAglChanged)

public:
    explicit ElevationMap(Fact *parent = nullptr);

    Fact *f_use;
    Fact *f_path;
    Fact *f_corridor;                        // profile corridor half-width, m
    static constexpr int AGL_PERIOD = 10000; // unit terrain request period, ms
    Fact *f_showAgl;                         // real-time AGL of the current unit
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
    bool available() const { return m_available; }
    bool covered() const { return m_covered; }
    bool active() const { return m_active; }
    double unitTerrain() const { return m_unitTerrain; }
    double unitAgl() const { return m_unitAgl; }
    // lowest terrain across the corridor (distance, elevation) for a profile path, empty if unknown
    QList<QPointF> minProfile(const QGeoPath &path) const;
    double elevation() const;
    void setElevation(double v);
    void getCorrectPathResponse(QList<QGeoCoordinate> v, int index);

protected:
    QGeoPath m_geoPath;
    QGeoCoordinate m_coordinate;
    double m_elevation;

private:
    QSharedPointer<AbstractElevationDB> m_elevationDB;
    QMap<int, QList<QGeoCoordinate>> m_correction;
    QMap<QString, int> m_waypoints;
    QSet<QString> m_runways;
    QSet<QString> m_pois;
    bool m_isCorrect{false};

    QSet<QString> m_tileNames; // elevation files in the configured directory
    bool m_available{false};
    bool m_covered{true};
    bool m_active{false};
    QTimer m_coverageTimer;

    QHash<QString, QList<QPointF>> m_minProfiles; // key: path endpoints
    static QString profileKey(const QGeoPath &path);

    double m_unitTerrain{qQNaN()};
    double m_unitAgl{qQNaN()};
    QTimer m_aglTimer; // periodic terrain request for the unit AGL

    bool hasTile(const QGeoCoordinate &c) const;

    void createDir(const QString &path);
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
    void getPluginEnableControl();
    void changeExternalsVisibility();
    void scanTiles();
    void scheduleCoverage();
    void updateCoverage();
    void updateActive();
    void onCorridorChanged();
    void onTerrainProfileMin(QGeoPath path, QList<double> minElevations);
    void updateUnitAgl();
    void setUnitTerrain(double elevation);
    void recalcUnitAgl();
    void setStartPointElevation();
    void startPathsCorrection();
    void correctUnsafePaths();
    void insertMissionWaypoints();
    void completeCorrection();
    void checkCorrectionResult();

signals:
    void coordinateChanged(QGeoCoordinate coordinate);
    void geoPathChanged(QGeoPath geoPath);
    void elevationChanged();
    void availableChanged();
    void coveredChanged();
    void activeChanged();
    void unitAglChanged();
};
