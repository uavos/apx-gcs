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
#include "VehiclesReqNode.h"

bool DBReqVehiclesNode::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM Nodes WHERE sn = ?");
    query.addBindValue(_uid);
    if (!query.exec())
        return false;
    if (query.next()) {
        nodeID = query.value(0).toULongLong();
        emit foundID(nodeID);
    }
    return true;
}

bool DBReqVehiclesSaveNodeInfo::run(QSqlQuery &query)
{
    if (!DBReqVehiclesNode::run(query))
        return false;

    if (!nodeID) {
        //register node
        query.prepare("INSERT INTO Nodes(sn) VALUES(?)");
        query.addBindValue(_uid);
        if (!query.exec())
            return false;
        nodeID = query.lastInsertId().toULongLong();
        qDebug() << "new node" << info.value("name").toString();
        emit foundID(nodeID);
    } else {
        //node already registered
        //qDebug()<<"node exists"<<info.value("name").toString();
        if (info.value("time").toULongLong() <= query.value("time").toULongLong()) {
            //info["time"]=query.value("time"); //imported
            return true; //no updates
        }
    }
    //set time to null on never seen nodes
    if (!info.value("time").toULongLong())
        info.remove("time");
    //uptime node info
    query.prepare("UPDATE Nodes SET time=?, name=?, version=?, hardware=? WHERE key=?");
    query.addBindValue(info.value("time"));
    query.addBindValue(info.value("name").toString());
    query.addBindValue(info.value("version").toString());
    query.addBindValue(info.value("hardware").toString());
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    return true;
}

bool DBReqVehiclesLoadNodeInfo::run(QSqlQuery &query)
{
    if (!DBReqVehiclesNode::run(query))
        return false;

    if (!nodeID)
        return true;

    query.prepare("SELECT * FROM Nodes WHERE key=?");
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    if (!query.next())
        return true;

    emit infoLoaded(queryRecord(query));
    return true;
}
