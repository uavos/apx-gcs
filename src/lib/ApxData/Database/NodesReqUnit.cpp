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
#include "NodesReqUnit.h"
#include "NodesReqConf.h"
#include "NodesReqDict.h"

using namespace db::nodes;

bool UnitSaveInfo::run(QSqlQuery &query)
{
    auto time = _info["time"].toVariant().toULongLong();
    auto name = _info["name"].toString();
    auto type = _info["type"].toString();

    auto uid = _info["uid"].toString();
    if (uid == QString(uid.size(), QChar('0')))
        uid.clear();
    if (!uid.isEmpty()) {
        if (type.isEmpty())
            type = "UAV";
    }

    //find the latest existing record by UID
    if (uid.isEmpty()) {
        query.prepare("SELECT * FROM Unit WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Unit WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(uid);
    }
    if (!query.exec())
        return false;

    while (query.next()) {
        auto latestTime = query.value("time").toULongLong();
        //merge ident from previous registrations
        if (name.isEmpty())
            name = query.value("name").toString();

        if (uid.isNull()) {
            //only one record with null uid (LOCAL)
            _unitID = query.value(0).toULongLong();
        } else {
            //find exact matching unique record
            query.prepare("SELECT * FROM Unit WHERE uid=? AND name=? AND type=?");
            query.addBindValue(uid);
            query.addBindValue(name);
            query.addBindValue(type);
            if (!query.exec())
                return false;
            if (!query.next())
                break; //create new
            _unitID = query.value(0).toULongLong();
            qDebug() << "unit exists" << name;
        }

        //same unit found - update data if newer
        if (latestTime >= time)
            return true;

        query.prepare("UPDATE Unit SET time=? WHERE key=?");
        query.addBindValue(time);
        query.addBindValue(_unitID);
        if (!query.exec())
            return false;
        emit foundID(_unitID);
        return true;
    }

    //register new unit record
    qDebug() << "new unit" << name;

    query.prepare("INSERT INTO Unit(uid,name,type,time) VALUES(?,?,?,?)");
    query.addBindValue(uid);
    query.addBindValue(name);
    query.addBindValue(type);
    query.addBindValue(time);
    if (!query.exec())
        return false;

    _unitID = query.lastInsertId().toULongLong();
    emit foundID(_unitID);

    return true;
}

bool UnitSaveConf::run(QSqlQuery &query)
{
    // seaarch unit
    if (_uid.isEmpty()) {
        query.prepare("SELECT * FROM Unit WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Unit WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(_uid);
    }
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "unit not found" << _uid;
        return true;
    }
    auto unitID = query.value(0).toULongLong();

    //generate hash
    //add hash from each node conf and sort nodes by uid
    QCryptographicHash h(QCryptographicHash::Sha1);
    QStringList stKeys;
    for (auto i : _nodeConfIDs)
        stKeys.append(QString::number(i));

    query.prepare("SELECT * FROM NodeConf "
                  "INNER JOIN NodeDict ON NodeConf.dictID=NodeDict.key "
                  "INNER JOIN Node ON NodeDict.nodeID=Node.key "
                  "WHERE NodeConf.key IN ("
                  + stKeys.join(',')
                  + ")"
                    "ORDER BY uid");
    if (!query.exec())
        return false;

    int kcnt = 0;
    while (query.next()) {
        kcnt++;
        h.addData(query.value("NodeConf.hash").toString().toUtf8());
        h.addData(query.value("Node.uid").toString().toUtf8());
    }
    if (kcnt <= 0) {
        qWarning() << "nothing to save";
        return true;
    }
    QString hash = h.result().toHex().toUpper();

    //find latest conf by hash
    query.prepare("SELECT * FROM UnitConf"
                  " LEFT JOIN Unit ON UnitConf.unitID=Unit.key"
                  " WHERE UnitConf.hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        // conf found
        auto unitConfID = query.value(0).toULongLong();
        //uptime time and actual values
        qDebug() << "unit conf exists" << query.value("name").toString() << _title;
        if (_time <= query.value("UnitConf.time").toULongLong())
            return true;

        query.prepare("UPDATE UnitConf SET time=?, unitID=?, notes=? WHERE key=?");
        query.addBindValue(_time);
        query.addBindValue(unitID);
        query.addBindValue(_notes);
        query.addBindValue(unitConfID);
        if (!query.exec())
            return false;
        emit dbModified();
        emit confSaved(hash, _title);
        return true;
    }

    //create new unit conf record
    db->transaction(query);

    query.prepare("INSERT INTO UnitConf(hash,time,title,unitID,notes) VALUES(?,?,?,?,?)");
    query.addBindValue(hash);
    query.addBindValue(_time);
    query.addBindValue(_title);
    query.addBindValue(unitID);
    query.addBindValue(_notes);
    if (!query.exec())
        return false;
    auto unitConfID = query.lastInsertId().toULongLong();

    //save unit conf bundle
    for (auto i : _nodeConfIDs) {
        query.prepare("INSERT INTO UnitConfData(unitConfID,nodeConfID) VALUES(?,?)");
        query.addBindValue(unitConfID);
        query.addBindValue(i);
        if (!query.exec())
            return false;
    }

    db->commit(query);
    emit dbModified();
    qDebug() << "new unit conf" << _title;
    emit confSaved(hash, _title);
    return true;
}

bool UnitLoadConf::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM UnitConf WHERE key=?");
    query.addBindValue(_unitConfID);
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "missing conf" << _unitConfID;
        return false;
    }
    auto unitConfID = query.value(0).toULongLong();
    auto title = query.value("title").toString();
    auto notes = query.value("notes").toString();
    auto time = query.value("time").toULongLong();
    auto unitID = query.value("unitID").toString();

    // get conf data
    query.prepare("SELECT * FROM UnitConfData"
                  " INNER JOIN NodeConf ON UnitConfData.nodeConfID=NodeConf.key"
                  " INNER JOIN NodeDict ON NodeConf.dictID=NodeDict.key"
                  " INNER JOIN Node ON NodeConf.nodeID=Node.key"
                  " WHERE UnitConfData.unitConfID=?");
    query.addBindValue(unitConfID);
    if (!query.exec())
        return false;

    // get list of nodes IDs
    QList<QList<quint64>> list;
    while (query.next()) {
        auto nodeID = query.value("nodeID").toULongLong();
        auto dictID = query.value("dictID").toULongLong();
        auto nodeConfID = query.value("nodeConfID").toULongLong();
        list.append({nodeID, dictID, nodeConfID});
    }
    if (list.isEmpty()) {
        qWarning() << "missing unit conf" << unitConfID;
        return true;
    }

    //qDebug()<<list.size();
    QJsonArray nodes;
    for (auto r : list) {
        QJsonObject node;
        {
            auto req = new NodeLoadInfo(r.value(0));
            bool ok = req->run(query);
            if (ok) {
                node["info"] = req->info();
            }
            delete req;
            if (!ok)
                return false;
        }
        {
            auto req = new NodeLoadDict(r.value(1));
            bool ok = req->run(query);
            if (ok) {
                node["dict"] = req->dict();
            }
            delete req;
            if (!ok)
                return false;
        }
        {
            auto req = new NodeLoadConf(r.value(2));
            bool ok = req->run(query);
            if (ok) {
                node["values"] = req->values();
                node["time"] = (qint64) req->time();
            }
            delete req;
            if (!ok)
                return false;
        }
        node = json::remove_empty(node);
        if (node.isEmpty())
            continue;

        nodes.append(node);
    }

    if (nodes.isEmpty()) {
        qWarning() << "no nodes in conf";
        return true;
    }

    _conf["nodes"] = nodes;

    if (!title.isEmpty())
        _conf["title"] = title;
    if (!notes.isEmpty())
        _conf["notes"] = notes;
    _conf["time"] = (qint64) time;

    // get unit info
    query.prepare("SELECT * FROM Unit WHERE key=?");
    query.addBindValue(unitID);
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "missing unit" << unitID;
    } else {
        _conf["unit"] = record_to_json(query.record(),
                                       {
                                           "uid",
                                           "name",
                                           "type",
                                           "time",
                                       });
    }

    emit confLoaded(_conf);
    return true;
}

bool UnitImportConf::run(QSqlQuery &query)
{
    // import unit
    auto unit = _conf["unit"].toObject();
    if (!unit.isEmpty()) {
        auto req = new UnitSaveInfo(unit);
        auto ok = req->run(query);
        delete req;
        if (!ok)
            return false;
    }

    // json::save("UnitImportConf-all", _conf);

    // import nodes
    QList<quint64> nodeConfIDs;
    DatabaseRequest *req = {};
    for (const auto &i : _conf["nodes"].toArray()) {
        auto node = i.toObject();

        // node info
        auto info = node["info"].toObject();
        if (info.isEmpty())
            continue;

        auto uid = info["uid"].toString();

        req = new NodeSaveInfo(info);
        if (!req->run(query))
            break;
        delete req;
        req = {};

        // node dict
        auto dict = node["dict"].toObject();
        if (dict.isEmpty())
            continue;

        req = new NodeSaveDict(uid, dict);
        if (!req->run(query))
            break;
        const auto dictID = static_cast<NodeSaveDict *>(req)->dictID();
        delete req;
        req = {};

        // node conf
        auto values = node["values"].toObject();
        if (values.isEmpty())
            continue;

        auto time = node["time"].toVariant().toULongLong();

        req = new NodeSaveConf(dictID, values, time);
        if (!req->run(query))
            break;
        nodeConfIDs.append(static_cast<NodeSaveConf *>(req)->nodeConfID());
        delete req;
        req = {};
    }
    if (req) {
        delete req;
        qWarning() << "failed to import unit conf";
        return true;
    }

    // unit conf
    auto uid = unit["uid"].toString();
    auto title = _conf["title"].toString();
    auto time = _conf["time"].toVariant().toULongLong();

    if (nodeConfIDs.isEmpty()) {
        qWarning() << "missing nodes in import";
    } else {
        QString notes("imported");

        req = new UnitSaveConf(uid, nodeConfIDs, title, notes, time);
        bool ok = req->run(query);
        delete req;
        if (!ok)
            return false;
    }

    return true;
}
