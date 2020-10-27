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

#include "VehiclesDB.h"
#include <QtCore>

class DBReqSaveVehicleInfo : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqSaveVehicleInfo(QVariantMap info)
        : DBReqVehicles()
        , vehicleID(0)
        , info(info)
        , t(QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}
    bool run(QSqlQuery &query);
    //result
    quint64 vehicleID;
    QVariantMap info;

private:
    quint64 t;
signals:
    void foundID(quint64 key);
};

class DBReqVehiclesSaveConfig : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveConfig(QList<quint64> nconfList,
                                     quint64 vehicleID,
                                     QString notes,
                                     quint64 t = 0)
        : DBReqVehicles()
        , nconfList(nconfList)
        , vehicleID(vehicleID)
        , notes(notes)
        , t(t ? t : static_cast<quint64>(QDateTime::currentDateTime().toMSecsSinceEpoch()))
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap configInfo;

private:
    QList<quint64> nconfList;
    quint64 vehicleID;
    QString notes;
    quint64 t;
signals:
    void configInfoFound(QVariantMap info);
    void configUpdated();
    void configCreated();
};

class DBReqVehiclesLoadConfig : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesLoadConfig(QString hash)
        : DBReqVehicles()
        , hash(hash)
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap configInfo;
    QList<QVariantMap> data;

private:
    QString hash;
signals:
    void loaded(QVariantMap configInfo, QList<QVariantMap> data);
};
