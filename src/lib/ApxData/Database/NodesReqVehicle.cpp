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
#include "NodesReqVehicle.h"
#include "NodesReqDict.h"
#include "NodesReqNconf.h"
//=============================================================================
bool DBReqSaveVehicleInfo::run(QSqlQuery &query)
{
    QString uid = info.value("uid").toString();
    QVariant vuid;
    if (uid == QString(uid.size(), QChar('0')))
        uid.clear();
    if (uid.isEmpty()) {
        info.remove("squawk");
    } else
        vuid = uid;
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
        if (info.value("class").toString().isEmpty())
            info["class"] = query.value("class");
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
        query.prepare("UPDATE Vehicles SET time=?, squawk=? WHERE key=?");
        query.addBindValue(info.value("time"));
        query.addBindValue(info.value("squawk"));
        query.addBindValue(vehicleID);
        if (!query.exec())
            return false;
        emit foundID(vehicleID);
        return true;
    }

    //register new vehicle record
    qDebug() << "new vehicle" << info.value("callsign").toString();

    query.prepare("INSERT INTO Vehicles(uid,callsign,class,time,squawk) VALUES(?,?,?,?,?)");
    query.addBindValue(vuid);
    query.addBindValue(info.value("callsign"));
    query.addBindValue(info.value("class"));
    query.addBindValue(info.value("time"));
    query.addBindValue(info.value("squawk"));
    if (!query.exec())
        return false;
    vehicleID = query.lastInsertId().toULongLong();
    emit foundID(vehicleID);
    return true;
}
//=============================================================================
bool DBReqNodesSaveConfig::run(QSqlQuery &query)
{
    configInfo.insert("time", t);
    if (vehicleID)
        configInfo.insert("vehicleID", vehicleID);

    //generate hash and find title
    QCryptographicHash h(QCryptographicHash::Sha1);
    //add hash from each node config and sort nodes by sn
    QStringList stKeys;
    for (int i = 0; i < nconfList.size(); ++i) {
        stKeys.append(QString::number(nconfList.at(i)));
    }
    query.prepare("SELECT * FROM NodeConfigs "
                  "INNER JOIN Nodes ON NodeConfigs.nodeID=Nodes.key "
                  "INNER JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key "
                  "WHERE NodeConfigs.key IN ("
                  + stKeys.join(',')
                  + ")"
                    "ORDER BY sn");
    if (!query.exec())
        return false;

    QMap<QString, QString> titleByName;
    QString titleShiva;
    QString titleLongest;
    QString titleAnyName;

    int kcnt = 0;
    while (query.next()) {
        kcnt++;
        //qDebug()<<hash<<kcnt;
        h.addData(query.value("NodeConfigs.hash").toString().toUtf8());
        h.addData(query.value("Nodes.sn").toString().toUtf8());
        //find title
        QString name = query.value("NodeDicts.name").toString();
        QString s = query.value("NodeConfigs.title").toString();
        if (titleAnyName.isEmpty())
            titleAnyName = name;
        if (s.isEmpty())
            continue;
        int sz = s.size();
        if (titleByName.value(name).size() < sz)
            titleByName[name] = s;
        if (name.endsWith(".shiva")) {
            if (titleShiva.size() < sz)
                titleShiva = s;
        }
        if (titleLongest.size() < sz)
            titleLongest = s;
    }
    if (kcnt <= 0) {
        qWarning() << "nothing to save";
        return true;
    }
    QString hash = h.result().toHex().toUpper();
    configInfo["hash"] = hash;
    //title
    QString title;
    while (1) {
        title = titleShiva;
        if (!title.isEmpty())
            break;
        title = titleByName.value("nav");
        if (!title.isEmpty())
            break;
        title = titleByName.value("mhx");
        if (!title.isEmpty())
            break;
        title = titleByName.value("ifc");
        if (!title.isEmpty())
            break;
        title = titleLongest;
        if (!title.isEmpty())
            break;
        title = titleAnyName;
        break;
    }
    configInfo["title"] = title;

    QString s = title.simplified();
    notes = notes.remove(s, Qt::CaseInsensitive).simplified();
    if (s.contains(notes))
        notes.clear();
    if (!notes.isEmpty())
        configInfo.insert("notes", notes);

    //qDebug()<<hash<<kcnt<<title;

    //find latest config by hash
    quint64 configID = 0;
    query.prepare("SELECT * FROM VehicleConfigs"
                  " LEFT JOIN Vehicles ON VehicleConfigs.vehicleID=Vehicles.key"
                  " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        configInfo.remove("key");
        configInfo = queryRecord(query, configInfo);
        //uptime time and actual values
        qDebug() << "config exists" << configInfo.value("callsign").toString() << title;
        if (query.value("VehicleConfigs.time").toULongLong()
            < configInfo.value("time").toULongLong()) {
            query.prepare("UPDATE VehicleConfigs SET time=?, vehicleID=?, notes=? WHERE key=?");
            query.addBindValue(configInfo.value("time"));
            query.addBindValue(configInfo.value("vehicleID"));
            query.addBindValue(configInfo.value("notes"));
            query.addBindValue(configInfo.value("key"));
            if (!query.exec())
                return false;
            emit dbModified();
            emit configUpdated();
        }
        emit configInfoFound(configInfo);
        return true;
    }

    //create new vehicle config record
    db->transaction(query);

    query.prepare("INSERT INTO VehicleConfigs(hash,time,title,vehicleID,notes) VALUES(?,?,?,?,?)");
    query.addBindValue(hash);
    query.addBindValue(configInfo.value("time"));
    query.addBindValue(configInfo.value("title"));
    query.addBindValue(configInfo.value("vehicleID"));
    query.addBindValue(configInfo.value("notes"));
    if (!query.exec())
        return false;
    configID = query.lastInsertId().toULongLong();
    configInfo["key"] = configID;

    //save vehicle config bundle
    for (int i = 0; i < nconfList.size(); ++i) {
        query.prepare("INSERT INTO VehicleConfigData(configID,nconfID) VALUES(?,?)");
        query.addBindValue(configID);
        query.addBindValue(nconfList.at(i));
        if (!query.exec())
            return false;
    }

    db->commit(query);
    emit dbModified();
    emit configInfoFound(configInfo);
    qDebug() << "new config" << title;
    emit configCreated();
    return true;
}
//=============================================================================
bool DBReqNodesLoadConfig::run(QSqlQuery &query)
{
    query.prepare("SELECT * FROM VehicleConfigs WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (!query.next()) {
        qWarning() << "missing hash" << hash;
        return false;
    }
    configInfo = queryRecord(query);
    quint64 configID = query.value("key").toULongLong();

    query.prepare("SELECT * FROM VehicleConfigData"
                  " INNER JOIN NodeConfigs ON VehicleConfigData.nconfID=NodeConfigs.key"
                  " INNER JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key"
                  " INNER JOIN Nodes ON NodeConfigs.nodeID=Nodes.key"
                  " WHERE VehicleConfigData.configID=?");
    query.addBindValue(configID);
    if (!query.exec())
        return false;
    QList<QPair<quint64, quint64>> list;
    while (query.next()) {
        quint64 dictID = query.value("dictID").toULongLong();
        quint64 nconfID = query.value("nconfID").toULongLong();
        list.append(QPair<quint64, quint64>(dictID, nconfID));
    }
    if (list.isEmpty()) {
        qWarning() << "missing vehicle config" << configID;
        return true;
    }
    //qDebug()<<list.size();
    for (int i = 0; i < list.size(); ++i) {
        QVariantMap dataItem;
        QPair<quint64, quint64> p = list.at(i);
        {
            DBReqNodesLoadDict *req = new DBReqNodesLoadDict(p.first);
            //connect(req,&DBReqLoadNodeDict::dictLoaded,this,&DBReqNodesLoadConfig::dictLoaded);
            bool ok = req->run(query);
            if (ok) {
                dataItem.insert("dictInfo", QVariant::fromValue(req->info));
                dataItem.insert("dict", QVariant::fromValue(req->dict));
            }
            delete req;
            if (!ok)
                return false;
        }
        {
            //collect configs to list for import
            DBReqNodesLoadNconf *req = new DBReqNodesLoadNconf(p.second);
            //connect(req,&DBReqNodesLoadNconf::configLoaded,this,&DBReqNodesLoadConfig::configLoaded);
            bool ok = req->run(query);
            if (ok) {
                dataItem.insert("nconfInfo", QVariant::fromValue(req->info));
                dataItem.insert("values", QVariant::fromValue(req->values));
                dataItem.insert("nconfID", p.second);
            }
            delete req;
            if (!ok)
                return false;
        }
        data.append(dataItem);
    }
    if (data.isEmpty())
        return true;
    //emit configInfoFound(configInfo);
    emit loaded(configInfo, data);
    return true;
}
//=============================================================================
