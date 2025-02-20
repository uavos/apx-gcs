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
#include <QtCore>

class AbstractElevationDB : public QObject
{
    Q_OBJECT

public:
    enum Utility {
        NONE = 0,
        GDALLOCATIONINFO,
    };

    AbstractElevationDB() = default;
    virtual double getElevation(double latitude, double longitude) = 0;
    virtual void setUtility(Utility u) = 0;
};

class OfflineElevationDB : public AbstractElevationDB
{
    Q_OBJECT

public:
    OfflineElevationDB(QString &path);
    double getElevation(double latitude, double longitude) override;
    double getElevationASTER(double latitude, double longitude); // Return NaN if the elevation is undefined
    void setUtility(Utility u) override;

private:
    QString m_dbPath;
    AbstractElevationDB::Utility m_utility;

    QString createASTERFileName(double latitude, double longitude);
    double getElevationFromGeoFile(QString fileName, double latitude, double longitude);
    char *SanitizeSRS(const char *userInput);

    bool checkGdallocationInfo();
    double getElevationFromGdallocationInfo(QString &fileName, double latitude, double longitude);
    QString getDataFromGdallocationInfo(QString &command);
};
