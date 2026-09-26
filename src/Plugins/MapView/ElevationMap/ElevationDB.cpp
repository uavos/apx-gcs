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
#include "ElevationDB.h"
#include "ElevationWorker.h"
#include <App/App.h>
#include <App/AppLog.h>

#include <cmath>

void AbstractElevationDB::receiveCoordinate(const QGeoCoordinate &coordinate)
{
    if (!coordinate.isValid()) {
        QGeoCoordinate checking(coordinate.latitude(), coordinate.longitude());
        if (!checking.isValid()) {
            apxMsgW() << tr("Invalid coordinate %1, %2, %3")
                             .arg(coordinate.latitude())
                             .arg(coordinate.longitude())
                             .arg(coordinate.altitude());
            return;
        }
    }
    emit coordinateReceived(coordinate);
}

OfflineElevationDB::OfflineElevationDB(const QString &path)
    : m_worker(new ElevationWorker(path, this))
{
    connect(m_worker,
            &ElevationWorker::elevationReady,
            this,
            &OfflineElevationDB::elevationReceived);
    connect(m_worker,
            &ElevationWorker::coordinateReady,
            this,
            &OfflineElevationDB::receiveCoordinate);
    connect(m_worker,
            &ElevationWorker::terrainProfileReady,
            this,
            &OfflineElevationDB::terrainProfileReceived);
    connect(m_worker,
            &ElevationWorker::terrainProfileMinReady,
            this,
            &OfflineElevationDB::terrainProfileMinReceived);
    connect(m_worker, &ElevationWorker::areaMaxReady, this, &OfflineElevationDB::areaMaxReceived);
}

OfflineElevationDB::~OfflineElevationDB()
{
    m_worker->stop();
}

QString OfflineElevationDB::createASTERFileName(double lat, double lon)
{
    int la = fabs(static_cast<int>(lat));
    int lo = fabs(static_cast<int>(lon));
    auto fileName = QString("ASTGTMV003_%1%2%3%4_dem.tif")
                        .arg((lat >= 0) ? 'N' : 'S')
                        .arg((lat >= 0 ? la : ++la), 2, 10, QChar('0'))
                        .arg((lon >= 0) ? 'E' : 'W')
                        .arg((lon >= 0 ? lo : ++lo), 3, 10, QChar('0'));
    return fileName;
}

void OfflineElevationDB::requestElevation(double latitude, double longitude)
{
    m_worker->requestElevation(latitude, longitude);
}

void OfflineElevationDB::requestCoordinate(double latitude, double longitude)
{
    m_worker->requestCoordinate(latitude, longitude);
}

void OfflineElevationDB::requestTerrainProfile(const QGeoPath &path)
{
    m_worker->requestTerrainProfile(path);
}

void OfflineElevationDB::requestAreaMax(double lat, double lon, double radius)
{
    m_worker->requestAreaMax(lat, lon, radius);
}

void OfflineElevationDB::setCorridor(double meters)
{
    m_worker->setCorridor(meters);
}

QGeoPath OfflineElevationDB::prepareRoute(const QGeoPath &path, double step)
{
    QGeoPath route;
    auto points = path.path();
    if (points.isEmpty())
        return route;
    if (step <= 0)
        step = TERRAIN_STEP;
    // Add path points
    for (int i = 0; i < points.size() - 1; ++i) {
        route.addCoordinate(points[i]);
        auto plotLenght = points[i].distanceTo(points[i + 1]);
        if (plotLenght > step) {
            double lenght{step};
            auto azimuth = points[i].azimuthTo(points[i + 1]);
            while (lenght < plotLenght) {
                auto point = points[i].atDistanceAndAzimuth(lenght, azimuth);
                route.addCoordinate(point);
                lenght += step;
            }
        }
    }
    // Add last point
    route.addCoordinate(points.last());
    return route;
}
