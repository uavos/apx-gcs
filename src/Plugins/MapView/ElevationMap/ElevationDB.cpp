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
#include <App/App.h>
#include <App/AppLog.h>

#include <QFile>
#include <QFuture>
#include <QtConcurrent>
#include <QMutexLocker>

#include <cmath>
#include <vector>

QMutex OfflineElevationDB::m_mutex;
QCache<QString, QImage> OfflineElevationDB::m_imageCache;

void AbstractElevationDB::receiveCoordinate(const QGeoCoordinate &coordinate)
{
    if(!coordinate.isValid()) {
        QGeoCoordinate checking(coordinate.latitude(), coordinate.longitude());
        if(!checking.isValid()) {
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
    : m_dbPath(path)
{
    m_imageCache.setMaxCost(CACHE_SIZE);
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

double OfflineElevationDB::getElevationTiffASTER(const QImage &image, const QString &file, double lat, double lon)
{
    double temp;
    auto modY = std::modf(lat, &temp);
    auto modX = std::modf(lon, &temp);
    if (modY < 0)
        modY++;
    if (modX < 0)
        modX++;

    double elevation{NAN};
    auto imageHeight = image.height();
    auto imageWidht = image.width();
    if (imageHeight == 0 || imageWidht == 0) {
        apxMsgW() << tr("Location is off this file").append(": ") << file;
        return elevation;
    }

    int pixelY = static_cast<int>(0.5 + std::abs((imageHeight - 1) * (1 - modY)));
    int pixelX = static_cast<int>(0.5 + std::abs((imageWidht - 1) * modX));
    if (pixelY >= imageHeight)
        pixelY = imageHeight - 1;
    if (pixelX >= imageWidht)
        pixelX = imageWidht - 1;

    uchar *src = const_cast<uchar *>(image.scanLine(pixelY));
    const short *line = reinterpret_cast<short *>(src);
    elevation = static_cast<double>(line[pixelX]);

    return elevation;
}

QGeoCoordinate OfflineElevationDB::requestCoordinateTiffASTER(const QImage &image, const QString &file, double lat, double lon)
{
    auto elv = getElevationTiffASTER(image, file, lat, lon);
    return QGeoCoordinate(lat, lon, elv);
}

void OfflineElevationDB::requestElevation(double latitude, double longitude)
{
    requestElevationASTER(latitude, longitude);
}

void OfflineElevationDB::requestCoordinate(double latitude, double longitude) {
    requestCoordinateASTER(latitude, longitude);
}

void OfflineElevationDB::requestElevationASTER(double latitude, double longitude)
{
    auto name = createASTERFileName(latitude, longitude);
    auto filePath = QString("%1/%2").arg(m_dbPath).arg(name);
    if (!QFile::exists(filePath)) {
        emit elevationReceived(qQNaN());
        return;
    }

    QFuture<double> future;
    setImage(filePath);
    future = QtConcurrent::run(getElevationTiffASTER, m_image, filePath, latitude, longitude);
    QFutureWatcher<double> *watcher = new QFutureWatcher<double>(this);
    connect(watcher, &QFutureWatcher<QGeoCoordinate>::finished, this, [watcher, this]() {
        auto result = watcher->result();
        emit elevationReceived(result);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void OfflineElevationDB::requestCoordinateASTER(double latitude, double longitude)
{
    auto name = createASTERFileName(latitude, longitude);
    auto filePath = QString("%1/%2").arg(m_dbPath).arg(name);
    if (!QFile::exists(filePath)) {
        emit coordinateReceived(QGeoCoordinate(latitude, longitude));
        return;
    }

    QFuture<QGeoCoordinate> future;
    setImage(filePath);
    future = QtConcurrent::run(requestCoordinateTiffASTER, m_image, filePath, latitude, longitude);
    QFutureWatcher<QGeoCoordinate> *watcher = new QFutureWatcher<QGeoCoordinate>(this);
    connect(watcher, &QFutureWatcher<QGeoCoordinate>::finished, this, [watcher, this]() {
        auto result = watcher->result();
        this->receiveCoordinate(result);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void OfflineElevationDB::setImage(const QString &file)
{
    if (file.isEmpty())
        return;

    if (m_fileName != file) {
        m_fileName = file;
        QFuture<QImage> future = QtConcurrent::run(getImageFromCache, file);
        m_image = future.result();
    }
}

// ===== Add route analyze ======
void OfflineElevationDB::requestTerrainProfile(const QGeoPath &path) {
    QFuture<QGeoPath> future;
    future = QtConcurrent::run(requestTerrainProfileASTER, path, m_dbPath);
    QFutureWatcher<QGeoPath> *watcher = new QFutureWatcher<QGeoPath>(this);
    connect(watcher, &QFutureWatcher<QGeoPath>::finished, this, [watcher, this]() {
        auto result = watcher->result();
        emit terrainProfileReceived(result);
        watcher->deleteLater();
    });
    connect(App::instance(), &App::appQuit, watcher, &QFutureWatcher<QGeoPath>::cancel);
    watcher->setFuture(future);
}

void OfflineElevationDB::requestTerrainProfileASTER(QPromise<QGeoPath> &promise,const QGeoPath &path, const QString &db)
{
    QImage image;
    QString imageFile;
    QGeoPath route = prepareRoute(path);
    for (qsizetype i = 0; i < route.size(); ++i) {
        // Stop request (if app closed)
        promise.suspendIfRequested();
        if (promise.isCanceled()) {
            return;
        }
        auto point = route.coordinateAt(i);
        double latitude = point.latitude();
        double longitude = point.longitude();
        
        auto file = createASTERFileName(latitude, longitude);
        auto filePath = QString("%1/%2").arg(db).arg(file);
        if (!QFile::exists(filePath)) {
            promise.addResult(path);
            return;
        }

        double elevation{0};
        auto image = getImageFromCache(filePath);
        elevation = getElevationTiffASTER(image, filePath, latitude, longitude);
        point.setAltitude(elevation);
        route.replaceCoordinate(i, point);
    }
    promise.addResult(route);
}

QGeoPath OfflineElevationDB::prepareRoute(const QGeoPath &path)
{
    QGeoPath route;
    auto points = path.path();
    // Add path points
    for (int i = 0; i < points.size() - 1; ++i) {
        route.addCoordinate(points[i]);
        auto plotLenght = points[i].distanceTo(points[i + 1]);
        if (plotLenght > TERRAIN_STEP) {
            double lenght{TERRAIN_STEP};
            auto azimuth = points[i].azimuthTo(points[i + 1]);
            while (lenght < plotLenght) {
                auto point = points[i].atDistanceAndAzimuth(lenght, azimuth);
                route.addCoordinate(point);
                lenght += TERRAIN_STEP;
            }
        }
    }
    // Add last point
    route.addCoordinate(points.last());
    return route;
}

QImage OfflineElevationDB::getImageFromCache(const QString &fileName) {
    QMutexLocker lock(&m_mutex);
    if(!m_imageCache.contains(fileName)) {
         QImage* imageToCache = new QImage(fileName);
         m_imageCache.insert(fileName, imageToCache);
    }
    return *m_imageCache.object(fileName);
}
