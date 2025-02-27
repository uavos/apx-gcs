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

class AbstractElevationDB : public QObject
{
    Q_OBJECT

public:
    enum Util {
        NONE = 0,
        GDALLOCATIONINFO,
    };

    AbstractElevationDB() = default;
    virtual double getElevation(double latitude, double longitude) = 0;
    virtual void setUtil(Util u) = 0;
};

class OfflineElevationDB : public AbstractElevationDB
{
    Q_OBJECT

public:
    OfflineElevationDB(const QString &path);
    double getElevation(double latitude, double longitude) override;
    double getElevationASTER(double latitude, double longitude); // Return NaN if the elevation is undefined
    void setUtil(Util u) override;

private:
    QImage m_image;
    QString m_dbPath;
    QString m_fileName;
    QString m_utilPath;
    QStringList m_paths;
    AbstractElevationDB::Util m_util;

    QString createASTERFileName(double latitude, double longitude);
    double getElevationFromTiffASTER(const QString &fileName, double latitude, double longitude);

    double getElevationFromGdallocationInfo(const QString &fileName, double latitude, double longitude);
    static QString getDataFromGdallocationInfo(const QString &command);
    QString searchUtil(const QString &name);
    void updateUtilPath();

    // Getting data from a geofile. 
    // Uses the gdal library, which supports the main geofile formats.
    // To use it, you need to include the gdal library in the project.
    double getElevationFromGeoFile(QString fileName, double latitude, double longitude);
    char *SanitizeSRS(const char *userInput);

signals:
    void utilChanged();
};
