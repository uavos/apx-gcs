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
#include "FleetReqUnit.h"
#include "FleetReqNode.h"

bool DBReqSaveUnitInfo::run(QSqlQuery &query)
{
    auto time = _info.value("time").toULongLong();
    auto callsign = _info.value("callsign").toString();
    auto vclass = _info.value("class").toString();

    auto vuid = _info.value("uid").toString();
    if (vuid == QString(vuid.size(), QChar('0')))
        vuid.clear();
    if (!vuid.isEmpty()) {
        if (vclass.isEmpty())
            vclass = "UAV";
    }

    //find the latest existing record by UID
    if (vuid.isEmpty()) {
        query.prepare("SELECT * FROM Fleet WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Fleet WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(vuid);
    }
    if (!query.exec())
        return false;

    while (query.next()) {
        auto latestTime = query.value("time").toULongLong();
        //merge ident from previous registrations
        if (callsign.isEmpty())
            callsign = query.value("callsign").toString();

        if (vuid.isNull()) {
            //only one record with null uid (LOCAL)
            unitID = query.value(0).toULongLong();
        } else {
            //find exact matching unique record
            query.prepare("SELECT * FROM Fleet WHERE uid=? AND callsign=? AND class=?");
            query.addBindValue(vuid);
            query.addBindValue(callsign);
            query.addBindValue(vclass);
            if (!query.exec())
                return false;
            if (!query.next())
                break; //create new
            unitID = query.value(0).toULongLong();
            qDebug() << "unit exists" << callsign;
        }

        //same unit found - update data if newer
        if (latestTime >= time)
            return true;

        query.prepare("UPDATE Fleet SET time=? WHERE key=?");
        query.addBindValue(time);
        query.addBindValue(unitID);
        if (!query.exec())
            return false;
        emit foundID(unitID);
        return true;
    }

    //register new unit record
    qDebug() << "new unit" << callsign;

    query.prepare("INSERT INTO Fleet(uid,callsign,class,time) VALUES(?,?,?,?)");
    query.addBindValue(vuid);
    query.addBindValue(callsign);
    query.addBindValue(vclass);
    query.addBindValue(time);
    if (!query.exec())
        return false;
    unitID = query.lastInsertId().toULongLong();
    emit foundID(unitID);
    return true;
}

bool DBReqSaveUnitConfig::run(QSqlQuery &query)
{
    // seaarch unit
    if (_vuid.isEmpty()) {
        query.prepare("SELECT * FROM Fleet WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Fleet WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(_vuid);
    }
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "no unit" << _vuid;
        return true;
    }
    auto unitID = query.value(0).toULongLong();

    //generate hash
    //add hash from each node config and sort nodes by uid
    QCryptographicHash h(QCryptographicHash::Sha1);
    QStringList stKeys;
    for (auto i : _nconfIDs)
        stKeys.append(QString::number(i));

    query.prepare("SELECT * FROM NodeConfigs "
                  "INNER JOIN Nodes ON NodeConfigs.nodeID=Nodes.key "
                  "INNER JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key "
                  "WHERE NodeConfigs.key IN ("
                  + stKeys.join(',')
                  + ")"
                    "ORDER BY sn");
    if (!query.exec())
        return false;

    int kcnt = 0;
    while (query.next()) {
        kcnt++;
        h.addData(query.value("NodeConfigs.hash").toString().toUtf8());
        h.addData(query.value("Nodes.sn").toString().toUtf8());
    }
    if (kcnt <= 0) {
        qWarning() << "nothing to save";
        return true;
    }
    QString hash = h.result().toHex().toUpper();

    //find latest config by hash
    query.prepare("SELECT * FROM UnitConfigs"
                  " LEFT JOIN Fleet ON UnitConfigs.unitID=Fleet.key"
                  " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        // config found
        auto configID = query.value(0).toULongLong();
        //uptime time and actual values
        qDebug() << "unit config exists" << query.value("callsign").toString() << _title;
        if (_time <= query.value("UnitConfigs.time").toULongLong())
            return true;

        query.prepare("UPDATE UnitConfigs SET time=?, unitID=?, notes=? WHERE key=?");
        query.addBindValue(_time);
        query.addBindValue(unitID);
        query.addBindValue(_notes);
        query.addBindValue(configID);
        if (!query.exec())
            return false;
        emit dbModified();
        emit configSaved(hash, _title);
        return true;
    }

    //create new unit config record
    db->transaction(query);

    query.prepare("INSERT INTO UnitConfigs(hash,time,title,unitID,notes) VALUES(?,?,?,?,?)");
    query.addBindValue(hash);
    query.addBindValue(_time);
    query.addBindValue(_title);
    query.addBindValue(unitID);
    query.addBindValue(_notes);
    if (!query.exec())
        return false;
    auto configID = query.lastInsertId().toULongLong();

    //save unit config bundle
    for (auto i : _nconfIDs) {
        query.prepare("INSERT INTO UnitConfigData(configID,nconfID) VALUES(?,?)");
        query.addBindValue(configID);
        query.addBindValue(i);
        if (!query.exec())
            return false;
    }

    db->commit(query);
    emit dbModified();
    qDebug() << "new unit config" << _title;
    emit configSaved(hash, _title);
    return true;
}

bool DBReqLoadUnitConfig::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM UnitConfigs WHERE hash=?");
    query.addBindValue(_hash);
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "missing config" << _hash;
        return false;
    }
    auto configID = query.value(0).toULongLong();
    auto title = query.value("title").toString();
    auto notes = query.value("notes").toString();
    auto time = query.value("time").toULongLong();
    auto unitID = query.value("unitID").toString();

    // get config data
    query.prepare("SELECT * FROM UnitConfigData"
                  " INNER JOIN NodeConfigs ON UnitConfigData.nconfID=NodeConfigs.key"
                  " INNER JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key"
                  " INNER JOIN Nodes ON NodeConfigs.nodeID=Nodes.key"
                  " WHERE UnitConfigData.configID=?");
    query.addBindValue(configID);
    if (!query.exec())
        return false;

    // get list of nodes IDs
    QList<QList<quint64>> list;
    while (query.next()) {
        auto nodeID = query.value("nodeID").toULongLong();
        auto dictID = query.value("dictID").toULongLong();
        auto nconfID = query.value("nconfID").toULongLong();
        list.append({nodeID, dictID, nconfID});
    }
    if (list.isEmpty()) {
        qWarning() << "missing unit config" << configID;
        return true;
    }

    //qDebug()<<list.size();
    QVariantList nodes;
    for (auto r : list) {
        QVariantMap node;
        {
            auto req = new DBReqLoadNodeInfo(r.value(0));
            bool ok = req->run(query);
            if (ok) {
                node.insert("info", req->info());
            }
            delete req;
            if (!ok)
                return false;
        }
        {
            auto req = new DBReqLoadNodeDict(r.value(1));
            bool ok = req->run(query);
            if (ok) {
                node.insert("dict", req->dict());
            }
            delete req;
            if (!ok)
                return false;
        }
        {
            auto req = new DBReqLoadNodeConfig(r.value(2));
            bool ok = req->run(query);
            if (ok) {
                node.insert("values", req->values());
                node.insert("time", req->time());
            }
            delete req;
            if (!ok)
                return false;
        }
        nodes.append(node);
    }
    _config.insert("nodes", nodes);
    if (!title.isEmpty())
        _config.insert("title", title);
    if (!notes.isEmpty())
        _config.insert("notes", notes);
    _config.insert("time", time);

    // get unit info
    query.prepare("SELECT * FROM Fleet WHERE key=?");
    query.addBindValue(unitID);
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "missing unit" << unitID;
    } else {
        QVariantMap unit;
        unit.insert("uid", query.value("uid").toString());
        unit.insert("callsign", query.value("callsign").toString());
        unit.insert("class", query.value("class").toString());
        unit.insert("time", query.value("time").toString());
        _config.insert("unit", filterNullValues(unit));
    }

    emit configLoaded(_config);
    return true;
}

bool DBReqImportUnitConfig::run(QSqlQuery &query)
{
    // import unit
    auto unit = _config.value("unit").value<QVariantMap>();
    if (!unit.isEmpty()) {
        auto req = new DBReqSaveUnitInfo(unit);
        auto ok = req->run(query);
        delete req;
        if (!ok)
            return false;
    }

    // import nodes
    QList<quint64> nconfIDs;
    DatabaseRequest *req = {};
    auto nodes = _config.value("nodes").value<QVariantList>();
    for (auto i : nodes) {
        auto node = i.value<QVariantMap>();

        // node info
        auto info = node.value("info").value<QVariantMap>();
        if (info.isEmpty())
            continue;
        auto uid = info.value("uid").toString();

        req = new DBReqSaveNodeInfo(info);
        if (!req->run(query))
            break;
        delete req;
        req = {};

        // node dict
        auto dict = node.value("dict").value<QVariantMap>();
        if (dict.isEmpty())
            continue;
        auto hash = dict.value("hash").toString();

        req = new DBReqSaveNodeDict(uid, dict);
        if (!req->run(query))
            break;
        delete req;
        req = {};

        // node config
        auto values = node.value("values").value<QVariantMap>();
        if (values.isEmpty())
            continue;
        auto time = node.value("time").toULongLong();

        req = new DBReqSaveNodeConfig(uid, hash, values, time);
        if (!req->run(query))
            break;
        nconfIDs.append(static_cast<DBReqSaveNodeConfig *>(req)->configID());
        delete req;
        req = {};
    }
    if (req) {
        delete req;
        qWarning() << "failed to import unit config";
        return true;
    }

    // unit config
    auto vuid = unit.value("uid").toString();
    auto title = _config.value("title").toString();
    auto time = _config.value("time").toULongLong();

    if (nconfIDs.isEmpty()) {
        qWarning() << "missing nodes in import";
    } else {
        QString notes("imported");

        req = new DBReqSaveUnitConfig(vuid, nconfIDs, title, notes, time);
        bool ok = req->run(query);
        delete req;
        if (!ok)
            return false;
    }

    return true;
}
