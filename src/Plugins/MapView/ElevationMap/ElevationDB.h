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

#include <QFutureWatcher>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QImage>
#include <QString>
#include <QtCore>
#include <QCache>
#include <QMutex>

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
    void requestElevation(double lat, double lon) override;
    void requestCoordinate(double lat, double lon) override;
    void requestTerrainProfile(const QGeoPath &path) override;
    double getElevationASTER(double lat, double lon); // Return NaN if the elevation is undefined

private:
    static constexpr int TERRAIN_STEP = 30; // terrain profile step in meters
    static constexpr int CACHE_SIZE = 10;   // image cache size
    static QCache<QString, QImage> m_imageCache;
    static QMutex m_mutex;
    QImage m_image;
    QString m_dbPath;
    QString m_fileName;
    QStringList m_paths;

    void setImage(const QString &file);
    void requestElevationASTER(double lat, double lon);
    void requestCoordinateASTER(double lat, double lon);
    static QString createASTERFileName(double lat, double lon);
    static QGeoCoordinate requestCoordinateTiffASTER(const QImage &image, const QString &file, double lat, double lon);
    static double getElevationTiffASTER(const QImage &image, const QString &file, double lat, double lon);
    static QImage getImageFromCache(const QString &fileName);
    static QGeoPath prepareRoute(const QGeoPath &path);
    static void requestTerrainProfileASTER(QPromise<QGeoPath> &promise, const QGeoPath &path, const QString &db);
};
