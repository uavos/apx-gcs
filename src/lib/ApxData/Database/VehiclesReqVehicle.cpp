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

bool DBReqSaveVehicleInfo::run(QSqlQuery &query)
{
    QString uid = info.value("uid").toString();
    QVariant vuid;
    if (uid == QString(uid.size(), QChar('0')))
        uid.clear();
    if (!uid.isEmpty()) {
        vuid = uid;
        if (info.value("class").isNull())
            info["class"] = "UAV";
    }
    if (!info.value("time").toULongLong())
        info["time"] = t;

    //find the latest existing record by UID
    if (vuid.isNull()) {
        query.prepare("SELECT * FROM Vehicles WHERE uid IS NULL ORDER BY time DESC LIMIT 1");
    } else {
        query.prepare("SELECT * FROM Vehicles WHERE uid=? ORDER BY time DESC LIMIT 1");
        query.addBindValue(vuid);
    }
    if (!query.exec())
        return false;
    while (query.next()) {
        quint64 latestTime = query.value("time").toULongLong();
        //merge ident from previous registrations
        if (info.value("callsign").toString().isEmpty())
            info["callsign"] = query.value("callsign");
        if (vuid.isNull()) {
            //only one record with null uid (LOCAL)
            vehicleID = query.value(0).toULongLong();
        } else {
            //find exact matching unique record
            query.prepare("SELECT * FROM Vehicles WHERE uid=? AND callsign=? AND class=?");
            query.addBindValue(vuid);
            query.addBindValue(info.value("callsign"));
            query.addBindValue(info.value("class"));
            if (!query.exec())
                return false;
            if (!query.next())
                break; //create new
            vehicleID = query.value(0).toULongLong();
            qDebug() << "vehicle exists" << info.value("callsign").toString();
        }
        //same vehicle found - update data if newer
        if (latestTime >= info.value("time").toULongLong())
            return true;
        query.prepare("UPDATE Vehicles SET time=? WHERE key=?");
        query.addBindValue(info.value("time"));
        query.addBindValue(vehicleID);
        if (!query.exec())
            return false;
        emit foundID(vehicleID);
        return true;
    }

    //register new vehicle record
    qDebug() << "new vehicle" << info.value("callsign").toString();

    query.prepare("INSERT INTO Vehicles(uid,callsign,class,time) VALUES(?,?,?,?)");
    query.addBindValue(vuid);
    query.addBindValue(info.value("callsign"));
    query.addBindValue(info.value("class"));
    query.addBindValue(info.value("time"));
    if (!query.exec())
        return false;
    vehicleID = query.lastInsertId().toULongLong();
    emit foundID(vehicleID);
    return true;
}

bool DBReqSaveVehicleConfig::run(QSqlQuery &query)
{
    // seaarch vehicle
    query.prepare("SELECT * FROM Vehicles WHERE uid=? ORDER BY time DESC LIMIT 1");
    query.addBindValue(_vuid);
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
        emit configSaved(configID);
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
    emit configSaved(configID);
    return true;
}
