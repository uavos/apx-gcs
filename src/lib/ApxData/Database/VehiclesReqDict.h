/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#pragma once

#include "VehiclesDB.h"
#include <Protocols/ProtocolNode.h>
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
    explicit DBReqVehiclesLoadDict(QString sn, QString chash)
        : DBReqVehicles(sn)
        , dictID(0)
        , chash(chash)
    {}
    explicit DBReqVehiclesLoadDict(quint64 dictID)
        : DBReqVehicles()
        , dictID(dictID)
    {}
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;
    DictNode::Dict dict;

private:
    quint64 dictID;
    QString chash;
signals:
    void dictInfoFound(QVariantMap dictInfo);
    void dictLoaded(QVariantMap info, DictNode::Dict dict);
};

class DBReqVehiclesSaveDict : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveDict(QVariantMap info, const ProtocolNode::Dict &dict)
        : DBReqVehicles(info.value("sn").toString())
        , info(info)
    {
        makeRecords(dict);
    }
    bool run(QSqlQuery &query);
    //result
    QVariantMap info;

private:
    Records records;
    void makeRecords(const ProtocolNode::Dict &dict);
signals:
    void dictInfoFound(QVariantMap dictInfo);
};
