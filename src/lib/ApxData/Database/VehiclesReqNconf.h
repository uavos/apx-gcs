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

class DBReqVehiclesLoadNconf : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesLoadNconf(quint64 nconfID)
        : DBReqVehicles()
        , nconfID(nconfID)
    {}
    explicit DBReqVehiclesLoadNconf(QString sn)
        : DBReqVehicles(sn)
        , nconfID(0)
    {}
    virtual bool run(QSqlQuery &query);

    //result
    QVariantMap info;
    QVariantMap values;

protected:
    quint64 nconfID;
signals:
    void nconfFound(quint64 nconfID);
    void configLoaded(QVariantMap info, QVariantMap values);
};

class DBReqVehiclesLoadNconfLatest : public DBReqVehiclesLoadNconf
{
    Q_OBJECT
public:
    explicit DBReqVehiclesLoadNconfLatest(QString sn)
        : DBReqVehiclesLoadNconf(sn)
    {}

protected:
    bool run(QSqlQuery &query);
};

class DBReqVehiclesSaveNconf : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveNconf(QVariantMap dictInfo, QVariantMap values, quint64 time)
        : DBReqVehicles(dictInfo.value("sn").toString())
        , nconfID(0)
        , dictInfo(dictInfo)
        , values(values)
        , time(time ? time : QDateTime::currentDateTime().toMSecsSinceEpoch())
    {}
    bool run(QSqlQuery &query);
    //result
    quint64 nconfID;

private:
    quint64 getValueID(QSqlQuery &query, const QVariant &v);
    QVariantMap dictInfo;
    QVariantMap values;
    quint64 time;
signals:
    void nconfFound(quint64 nconfID);
};
