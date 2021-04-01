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

class DBReqVehiclesNode : public DBReqVehicles
{
    Q_OBJECT
public:
    explicit DBReqVehiclesNode(QString uid)
        : DBReqVehicles()
        , _uid(uid)
    {}

    virtual bool run(QSqlQuery &query) override;

    quint64 nodeID{};

protected:
    QString _uid;

signals:
    void foundID(quint64 key);
};

class DBReqVehiclesSaveNodeInfo : public DBReqVehiclesNode
{
    Q_OBJECT
public:
    explicit DBReqVehiclesSaveNodeInfo(QVariantMap info)
        : DBReqVehiclesNode(info.value("uid").toString())
        , info(info)
    {}

    bool run(QSqlQuery &query) override;

private:
    QVariantMap info;
};

class DBReqVehiclesLoadNodeInfo : public DBReqVehiclesNode
{
    Q_OBJECT
public:
    explicit DBReqVehiclesLoadNodeInfo(QString uid)
        : DBReqVehiclesNode(uid)
    {}

    bool run(QSqlQuery &query) override;

signals:
    void infoLoaded(QVariantMap info);
};
