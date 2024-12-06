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
#include "NodesReq.h"

using namespace db::nodes;

bool RequestNode::run(QSqlQuery &query)
{
    if (!_nodeID) {
        query.prepare("SELECT * FROM Node WHERE uid=?");
        query.addBindValue(_uid);
        if (!query.exec())
            return false;
        if (query.next()) {
            _nodeID = query.value(0).toULongLong();
            emit foundID(_nodeID);
        }
    } else {
        query.prepare("SELECT * FROM Node WHERE key=?");
        query.addBindValue(_nodeID);
        if (!query.exec())
            return false;
        if (query.next()) {
            _uid = query.value("uid").toString();
        }
    }
    return true;
}

bool NodeSaveInfo::run(QSqlQuery &query)
{
    if (!RequestNode::run(query))
        return false;

    auto time = _info["time"].toInteger();
    auto name = _info["name"].toString();

    if (!_nodeID) {
        //register node
        query.prepare("INSERT INTO Node(uid) VALUES(?)");
        query.addBindValue(_uid);
        if (!query.exec())
            return false;
        _nodeID = query.lastInsertId().toULongLong();
        qDebug() << "new node" << name;
        emit foundID(_nodeID);
    } else {
        // check for imported node in the past vs recently seen
        if (time <= query.value("time").toULongLong()) {
            qDebug() << "node import skipped" << name;
            return true;
        }
    }

    //uptime node info
    query.prepare("UPDATE Node SET time=?, name=?, version=?, hardware=?"
                  " WHERE key=?");
    query.addBindValue(time ? time : QVariant());
    query.addBindValue(name);
    query.addBindValue(_info["version"]);
    query.addBindValue(_info["hardware"]);
    query.addBindValue(_nodeID);
    if (!query.exec())
        return false;

    return true;
}

bool NodeLoadInfo::run(QSqlQuery &query)
{
    if (!RequestNode::run(query))
        return false;

    if (!_nodeID)
        return true;

    QJsonObject jso;
    jso = record_to_json(query.record(),
                         {
                             "time",
                             "name",
                             "version",
                             "hardware",
                         });
    jso["uid"] = _uid;

    _info = json::filter_names(jso);
    emit infoLoaded(_info);
    return true;
}
