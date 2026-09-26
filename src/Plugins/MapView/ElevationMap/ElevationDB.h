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
#include <QString>
#include <QtCore>

class ElevationWorker;

class AbstractElevationDB : public QObject
{
    Q_OBJECT

public:
    AbstractElevationDB() = default;
    virtual void requestElevation(double lat, double lon) = 0;
    virtual void requestCoordinate(double lat, double lon) = 0;
    virtual void requestTerrainProfile(const QGeoPath &path) = 0;
    // highest terrain within radius [m] around the point (unit AGL)
    virtual void requestAreaMax(double lat, double lon, double radius) = 0;
    // corridor half-width [m] used by the terrain profile
    virtual void setCorridor(double meters) = 0;

protected:
    virtual void receiveCoordinate(const QGeoCoordinate &coordinate);

signals:
    void coordinateReceived(QGeoCoordinate coordinate);
    void elevationReceived(double elevation);
    void terrainProfileReceived(QGeoPath path);
    // lowest terrain across the corridor for every point of the profile (the path altitude is the corridor max)
    void terrainProfileMinReceived(QGeoPath path, QList<double> minElevations);
    void areaMaxReceived(double elevation);
};

class OfflineElevationDB : public AbstractElevationDB
{
    Q_OBJECT

public:
    OfflineElevationDB(const QString &path);
    ~OfflineElevationDB() override;
    void requestElevation(double lat, double lon) override;
    void requestCoordinate(double lat, double lon) override;
    void requestTerrainProfile(const QGeoPath &path) override;
    void requestAreaMax(double lat, double lon, double radius) override;
    void setCorridor(double meters) override;

    static constexpr int TERRAIN_STEP = 30; // default terrain profile step in meters
    static QString createASTERFileName(double lat, double lon);
    // path with intermediate points every `step` meters (the map resolution)
    static QGeoPath prepareRoute(const QGeoPath &path, double step = TERRAIN_STEP);

private:
    // all tile reading and elevation lookups run in this thread
    ElevationWorker *m_worker;
};
