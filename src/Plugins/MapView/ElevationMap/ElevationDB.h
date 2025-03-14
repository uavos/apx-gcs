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

#include <QString>
#include <QImage>
#include <QtCore>
#include <QGeoCoordinate>
#include <QFutureWatcher>

class AbstractElevationDB : public QObject
{
    Q_OBJECT

public:
    enum Util {
        NONE = 0,
        GDALLOCATIONINFO,
    };

    AbstractElevationDB() = default;
    virtual void requestElevation(double lat, double lon) = 0;
    virtual void requestCoordinate(double lat, double lon) = 0;
    virtual void setUtil(Util u) = 0;

protected:
    virtual void receiveCoordinate(const QGeoCoordinate &coordinate);

signals:
    void coordinateReceived(QGeoCoordinate coordinate);
    void elevationReceived(double elevation);
};

class OfflineElevationDB : public AbstractElevationDB
{
    Q_OBJECT

public:
    OfflineElevationDB(const QString &path);
    void requestElevation(double lat, double lon) override;
    void requestCoordinate(double lat, double lon) override;
    double getElevationASTER(double lat, double lon); // Return NaN if the elevation is undefined
    void setUtil(Util u) override;

private:
    QImage m_image;
    QString m_dbPath;
    QString m_fileName;
    QString m_utilPath;
    QStringList m_paths;
    AbstractElevationDB::Util m_util;

    void updateUtilPath();
    void setImage(const QString &file);
    void requestElevationASTER(double lat, double lon);
    void requestCoordinateASTER(double lat, double lon);
    QString createASTERFileName(double lat, double lon);
    QString searchUtil(const QString &name);
    static QGeoCoordinate requestCoordinateGdallocationInfo(const QString &util, const QString &file, double lat, double lon);
    static QGeoCoordinate requestCoordinateTiffASTER(const QImage &image, const QString &file, double lat, double lon);
    static double getElevationTiffASTER(const QImage &image, const QString &file, double lat, double lon);
    static double getElevationGdallocationInfo(const QString &util, const QString &file, double lat, double lon);
    static QString getDataFromGdallocationInfo(const QString &command);

    // Getting data from a geofile. 
    // Uses the gdal library, which supports the main geofile formats.
    // To use it, you need to include the gdal library in the project.
    static QGeoCoordinate requestCoordinateFromGeoFile(const QString &file, double lat, double lon);
    static double getElevationFromGeoFile(QString file, double lat, double lon);
    static char *SanitizeSRS(const char *userInput);

signals:
    void utilChanged();
};
