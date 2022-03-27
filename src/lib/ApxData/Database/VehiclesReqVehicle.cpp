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
#include "VehiclesReqVehicle.h"
#include "VehiclesReqNode.h"

bool DBReqSaveVehicleInfo::run(QSqlQuery &query)
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
        query.prepare("SELECT * FROM Vehicles WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Vehicles WHERE uid=? ORDER BY time DESC LIMIT 1");
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
            vehicleID = query.value(0).toULongLong();
        } else {
            //find exact matching unique record
            query.prepare("SELECT * FROM Vehicles WHERE uid=? AND callsign=? AND class=?");
            query.addBindValue(vuid);
            query.addBindValue(callsign);
            query.addBindValue(vclass);
            if (!query.exec())
                return false;
            if (!query.next())
                break; //create new
            vehicleID = query.value(0).toULongLong();
            qDebug() << "vehicle exists" << callsign;
        }

        //same vehicle found - update data if newer
        if (latestTime >= time)
            return true;

        query.prepare("UPDATE Vehicles SET time=? WHERE key=?");
        query.addBindValue(time);
        query.addBindValue(vehicleID);
        if (!query.exec())
            return false;
        emit foundID(vehicleID);
        return true;
    }

    //register new vehicle record
    qDebug() << "new vehicle" << callsign;

    query.prepare("INSERT INTO Vehicles(uid,callsign,class,time) VALUES(?,?,?,?)");
    query.addBindValue(vuid);
    query.addBindValue(callsign);
    query.addBindValue(vclass);
    query.addBindValue(time);
    if (!query.exec())
        return false;
    vehicleID = query.lastInsertId().toULongLong();
    emit foundID(vehicleID);
    return true;
}

bool DBReqSaveVehicleConfig::run(QSqlQuery &query)
{
    // seaarch vehicle
    if (_vuid.isEmpty()) {
        query.prepare("SELECT * FROM Vehicles WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Vehicles WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(_vuid);
    }
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "no vehicle" << _vuid;
        return true;
    }
    auto vehicleID = query.value(0).toULongLong();

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
    query.prepare("SELECT * FROM VehicleConfigs"
                  " LEFT JOIN Vehicles ON VehicleConfigs.vehicleID=Vehicles.key"
                  " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        // config found
        auto configID = query.value(0).toULongLong();
        //uptime time and actual values
        qDebug() << "vehicle config exists" << query.value("callsign").toString() << _title;
        if (_time <= query.value("VehicleConfigs.time").toULongLong())
            return true;

        query.prepare("UPDATE VehicleConfigs SET time=?, vehicleID=?, notes=? WHERE key=?");
        query.addBindValue(_time);
        query.addBindValue(vehicleID);
        query.addBindValue(_notes);
        query.addBindValue(configID);
        if (!query.exec())
            return false;
        emit dbModified();
        emit configSaved(hash, _title);
        return true;
    }

    //create new vehicle config record
    db->transaction(query);

    query.prepare("INSERT INTO VehicleConfigs(hash,time,title,vehicleID,notes) VALUES(?,?,?,?,?)");
    query.addBindValue(hash);
    query.addBindValue(_time);
    query.addBindValue(_title);
    query.addBindValue(vehicleID);
    query.addBindValue(_notes);
    if (!query.exec())
        return false;
    auto configID = query.lastInsertId().toULongLong();

    //save vehicle config bundle
    for (auto i : _nconfIDs) {
        query.prepare("INSERT INTO VehicleConfigData(configID,nconfID) VALUES(?,?)");
        query.addBindValue(configID);
        query.addBindValue(i);
        if (!query.exec())
            return false;
    }

    db->commit(query);
    emit dbModified();
    qDebug() << "new vehicle config" << _title;
    emit configSaved(hash, _title);
    return true;
}

bool DBReqLoadVehicleConfig::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM VehicleConfigs WHERE hash=?");
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
    auto vehicleID = query.value("vehicleID").toString();

    // get config data
    query.prepare("SELECT * FROM VehicleConfigData"
                  " INNER JOIN NodeConfigs ON VehicleConfigData.nconfID=NodeConfigs.key"
                  " INNER JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key"
                  " INNER JOIN Nodes ON NodeConfigs.nodeID=Nodes.key"
                  " WHERE VehicleConfigData.configID=?");
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
        qWarning() << "missing vehicle config" << configID;
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

    // get vehicle info
    query.prepare("SELECT * FROM Vehicles WHERE key=?");
    query.addBindValue(vehicleID);
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "missing vehicle" << vehicleID;
    } else {
        QVariantMap vehicle;
        vehicle.insert("uid", query.value("uid").toString());
        vehicle.insert("callsign", query.value("callsign").toString());
        vehicle.insert("class", query.value("class").toString());
        vehicle.insert("time", query.value("time").toString());
        _config.insert("vehicle", filterNullValues(vehicle));
    }

    emit configLoaded(_config);
    return true;
}

bool DBReqImportVehicleConfig::run(QSqlQuery &query)
{
    // import vehicle
    auto vehicle = _config.value("vehicle").value<QVariantMap>();
    if (!vehicle.isEmpty()) {
        auto req = new DBReqSaveVehicleInfo(vehicle);
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
        qWarning() << "failed to import vehicle config";
        return true;
    }

    // vehicle config
    auto vuid = vehicle.value("uid").toString();
    auto title = _config.value("title").toString();
    auto time = _config.value("time").toULongLong();

    if (nconfIDs.isEmpty()) {
        qWarning() << "missing nodes in import";
    } else {
        QString notes("imported");

        req = new DBReqSaveVehicleConfig(vuid, nconfIDs, title, notes, time);
        bool ok = req->run(query);
        delete req;
        if (!ok)
            return false;
    }

    return true;
}
