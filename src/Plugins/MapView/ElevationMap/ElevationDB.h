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

protected:
    virtual void receiveCoordinate(const QGeoCoordinate &coordinate);

signals:
    void coordinateReceived(QGeoCoordinate coordinate);
    void elevationReceived(double elevation);
    void terrainProfileReceived(QGeoPath path);
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

    static constexpr int TERRAIN_STEP = 30; // terrain profile step in meters
    static QString createASTERFileName(double lat, double lon);
    static QGeoPath prepareRoute(const QGeoPath &path);

private:
    // all tile reading and elevation lookups run in this thread
    ElevationWorker *m_worker;
};
