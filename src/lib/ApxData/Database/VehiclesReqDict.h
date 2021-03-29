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

class DBReqVehiclesLoadInfo : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesLoadInfo(QString sn)
        : DBReqVehicles(sn)
    {}

protected:
    bool run(QSqlQuery &query);
signals:
    void infoLoaded(QVariantMap info);
};

class DBReqVehiclesSaveInfo : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveInfo(QVariantMap info)
        : DBReqVehicles(info.value("sn").toString())
        , info(info)
    {}
    bool run(QSqlQuery &query);

private:
    QVariantMap info;
};

class DBReqVehiclesSaveUser : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveUser(QString sn, QVariantMap info, qint64 t = 0)
        : DBReqVehicles(sn)
        , info(info)
        , t(t > 0 ? t : QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}
    bool run(QSqlQuery &query);

private:
    QVariantMap info;
    qint64 t;
};
class DBReqVehiclesLoadUser : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesLoadUser(QString sn)
        : DBReqVehicles(sn)
    {}
    //result
    QVariantMap info;
    bool run(QSqlQuery &query);
};

class DBReqVehiclesLoadDict : public DBReqVehicles
{
    Q_OBJECT
public:
    //load cache
    explicit DBReqVehiclesLoadDict(QString sn, QString hash)
        : DBReqVehicles(sn)
        , dictID(0)
        , hash(hash)
    {}
    explicit DBReqVehiclesLoadDict(quint64 dictID)
        : DBReqVehicles()
        , dictID(dictID)
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;
    //ProtocolNode::Dict dict;

private:
    quint64 dictID;
    QString hash;
signals:
    void dictInfoFound(QVariantMap dictInfo);
    //void dictLoaded(QVariantMap info, const ProtocolNode::Dict &dict);
};

class DBReqVehiclesSaveDict : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveDict(QVariantMap info, QJsonValue json)
        : DBReqVehicles(info.value("sn").toString())
        , info(info)
    {
        //makeRecords(dict);
    }
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;

private:
    Records records;
    //void makeRecords(const ProtocolNode::Dict &dict);
signals:
    void dictInfoFound(QVariantMap dictInfo);
};
