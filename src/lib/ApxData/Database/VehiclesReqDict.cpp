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
#include "VehiclesReqDict.h"

bool DBReqVehiclesLoadInfo::run(QSqlQuery &query)
{
    if (!DBReqVehicles::run(query))
        return false; //get nodeID
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

bool DBReqVehiclesSaveInfo::run(QSqlQuery &query)
{
    if (!DBReqVehicles::run(query))
        return false; //get nodeID
    if (!nodeID) {
        //register node
        query.prepare("INSERT INTO Nodes(sn) VALUES(?)");
        query.addBindValue(sn);
        if (!query.exec())
            return false;
        nodeID = query.lastInsertId().toULongLong();
        qDebug() << "new node" << info.value("name").toString();
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

bool DBReqVehiclesSaveUser::run(QSqlQuery &query)
{
    if (!DBReqVehicles::run(query))
        return false; //get nodeID
    if (!nodeID) {
        qDebug() << "missing node" << sn;
        return true;
    }
    //find existing record
    query.prepare("SELECT * FROM NodeUsers WHERE nodeID=?");
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    if (query.next()) {
        //update time only
        quint64 key = query.value(0).toULongLong();
        if (t <= query.value("time").toLongLong())
            return true; //imported
        query.prepare("UPDATE NodeUsers"
                      " SET time=?,machineUID=?,hostname=?,username=?"
                      " WHERE key=?");
        query.addBindValue(t);
        query.addBindValue(info.value("machineUID"));
        query.addBindValue(info.value("hostname"));
        query.addBindValue(info.value("username"));
        query.addBindValue(key);
        if (!query.exec())
            return false;
        //qDebug()<<"node user exists";
        return true;
    }
    //register user for node
    query.prepare("INSERT INTO NodeUsers"
                  "(nodeID,time,firstTime,machineUID,hostname,username)"
                  " VALUES(?,?,?,?,?,?)");
    query.addBindValue(nodeID);
    query.addBindValue(t);
    query.addBindValue(t);
    query.addBindValue(info.value("machineUID"));
    query.addBindValue(info.value("hostname"));
    query.addBindValue(info.value("username"));
    if (!query.exec())
        return false;
    qDebug() << "new node user" << info.value("username").toString()
             << info.value("hostname").toString();
    return true;
}
bool DBReqVehiclesLoadUser::run(QSqlQuery &query)
{
    if (!DBReqVehicles::run(query))
        return false; //get nodeID
    if (!nodeID) {
        qDebug() << "missing node" << sn;
        return true;
    }
    //find existing record
    query.prepare("SELECT * FROM NodeUsers WHERE nodeID=?");
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    if (!query.next())
        return true;
    info = filterIdValues(queryRecord(query));
    return true;
}

bool DBReqVehiclesLoadDict::run(QSqlQuery &query)
{
    //QTime t0;
    //t0.start();
    if (dictID) {
        query.prepare("SELECT * FROM NodeDicts "
                      "INNER JOIN Nodes ON NodeDicts.nodeID=Nodes.key "
                      "WHERE NodeDicts.key=?");
        query.addBindValue(dictID);
        if (!query.exec())
            return false;
        if (!query.next()) {
            qWarning() << "missing node dict by ID" << dictID;
            return true;
        }
    }
    if (!sn.isEmpty()) {
        if (!DBReqVehicles::run(query))
            return false; //get nodeID
        if (!nodeID)
            return true;
        if (!hash.isEmpty()) {
            //find existing dictionary by chash
            query.prepare("SELECT * FROM NodeDicts "
                          "INNER JOIN Nodes ON NodeDicts.nodeID=Nodes.key "
                          "WHERE nodeID=? AND hash=? "
                          "ORDER BY NodeDicts.time DESC, NodeDicts.key DESC "
                          "LIMIT 1");
            query.addBindValue(nodeID);
            query.addBindValue(hash);
            if (!query.exec())
                return false;
            if (!query.next()) {
                qDebug() << "cache not exists";
                return true;
            }
            dictID = query.value(0).toULongLong();
        }
    }

    if (!dictID) {
        qWarning() << "missing node dict" << sn << hash;
        return true;
    }

    info = queryRecord(query);

    //read fields
    query.prepare("SELECT * FROM NodeDictData "
                  "INNER JOIN NodeDictDataFields ON NodeDictData.fieldID=NodeDictDataFields.key "
                  "WHERE dictID=? "
                  "ORDER BY fidx ASC");
    query.addBindValue(dictID);
    if (!query.exec())
        return false;
    QHash<QString, xbus::node::conf::type_e> types;
    for (uint8_t i = 0; i < xbus::node::conf::type_max; ++i) {
        QString s(xbus::node::conf::type_to_str(static_cast<xbus::node::conf::type_e>(i)));
        if (s.isEmpty())
            continue;
        types.insert(s, static_cast<xbus::node::conf::type_e>(i));
    }

    while (query.next()) {
        ProtocolNode::dict_field_s f;
        f.path = query.value("name").toString();
        f.name = f.path.split('.').last();
        f.title = query.value("title").toString();
        f.units = query.value("units").toString();
        f.type = types.value(query.value("type").toString());
        f.array = query.value("array").toUInt();
        f.group = query.value("gidx").toUInt();
        dict.append(f);
    }

    //qDebug()<<t0.elapsed()<<"ms";
    emit dictInfoFound(info);
    emit dictLoaded(info, dict);
    return true;
}

void DBReqVehiclesSaveDict::makeRecords(const ProtocolNode::Dict &dict)
{
    records.names << "name"
                  << "title"
                  << "units"
                  << "type"
                  << "array"
                  << "fidx"
                  << "gidx";

    uint16_t fid = 0;
    for (auto const &i : dict) {
        QVariantList v;
        v << i.path;
        v << (i.title.isEmpty() ? QVariant() : i.title);
        v << (i.units.isEmpty() ? QVariant() : i.units);
        v << QString(xbus::node::conf::type_to_str(i.type));
        v << (i.array > 0 ? i.array : QVariant());
        v << fid++;
        v << i.group;
        records.values.append(v);
    }
}
bool DBReqVehiclesSaveDict::run(QSqlQuery &query)
{
    if (info.contains("nodeID"))
        nodeID = info.value("nodeID").toULongLong();
    else if (!DBReqVehicles::run(query))
        return false; //get nodeID
    else
        info = queryRecord(query, info);
    if (!nodeID)
        return false;

    //generate hash
    if (!info.contains("hash")) {
        QCryptographicHash h(QCryptographicHash::Sha1);
        h.addData(info.value("name").toString().toUtf8());
        h.addData(info.value("version").toString().toUtf8());
        h.addData(info.value("hardware").toString().toUtf8());
        getHash(h, records);
        info.insert("hash", h.result().toHex().toUpper());
    }

    //find existing dictionary
    quint64 dictID = 0;
    query.prepare(
        "SELECT * FROM NodeDicts WHERE nodeID=? AND hash=? ORDER BY time DESC, key DESC LIMIT 1");
    query.addBindValue(nodeID);
    query.addBindValue(info.value("hash").toString());
    if (!query.exec())
        return false;

    if (query.next()) {
        //dictionary exists
        qDebug() << "dict exists";
        dictID = query.value(0).toULongLong();
        info["key"] = dictID;
        if (info.value("time").toULongLong()) {
            //uptime existing dict record of actual synced node
            if (info.value("time").toULongLong() < query.value("time").toULongLong())
                info.insert("time", query.value("time").toULongLong());
            if (info.value("hash").toString().isEmpty())
                info.insert("hash", query.value("hash").toString());
            query.prepare("UPDATE NodeDicts SET time=?, hash=? WHERE key = ?");
            query.addBindValue(info.value("time").toULongLong());
            query.addBindValue(info.value("hash").toString());
            query.addBindValue(dictID);
            if (!query.exec())
                return false;
        } else
            info = queryRecord(query, info);
        emit dictInfoFound(info);
        return true;
    }

    //create new dict record
    db->transaction(query);
    query.prepare("INSERT INTO NodeDicts(nodeID,time,hash,name,version,hardware) "
                  "VALUES(?,?,?,?,?,?)");
    query.addBindValue(nodeID);
    query.addBindValue(info.value("time").toULongLong());
    query.addBindValue(info.value("hash").toString());
    query.addBindValue(info.value("name").toString());
    query.addBindValue(info.value("version").toString());
    query.addBindValue(info.value("hardware").toString());
    if (!query.exec())
        return false;
    dictID = query.lastInsertId().toULongLong();
    info["key"] = dictID;

    //uptime fields records
    QList<quint64> fieldIDs;
    QStringList fnames;
    fnames << "name"
           << "title"
           << "units"
           << "type"
           << "array";
    for (int i = 0; i < records.values.size(); ++i) {
        QVariantList vlist;
        for (int j = 0; j < fnames.size(); ++j) {
            vlist.append(records.values.at(i).at(records.names.indexOf(fnames.at(j))));
        }

        QStringList fnamesNull;
        QStringList fnamesNotNull;
        for (int j = 0; j < vlist.size(); ++j) {
            if (vlist.at(j).isNull())
                fnamesNull.append(fnames.at(j));
            else
                fnamesNotNull.append(fnames.at(j));
        }

        QString s1 = fnamesNotNull.join("=? AND ") + "=?";
        QString s2 = fnamesNull.isEmpty()
                         ? ""
                         : (" AND " + fnamesNull.join(" IS NULL AND ") + " IS NULL");
        query.prepare("SELECT key FROM NodeDictDataFields "
                      " WHERE "
                      + s1 + s2);
        for (int j = 0; j < vlist.size(); ++j) {
            if (!vlist.at(j).isNull())
                query.addBindValue(vlist.at(j));
        }
        if (!query.exec())
            return false;
        if (query.next()) {
            fieldIDs.append(query.value(0).toULongLong());
            continue;
        }

        query.prepare("INSERT INTO NodeDictDataFields(" + fnames.join(',')
                      + ") "
                        "VALUES(?,?,?,?,?)");
        for (int j = 0; j < fnames.size(); ++j) {
            query.addBindValue(vlist.at(j));
        }
        if (!query.exec())
            return false;
        //qDebug()<<"new field"<<vlist.at(0).toString();
        fieldIDs.append(query.lastInsertId().toULongLong());
    }

    //write dict fields
    for (int i = 0; i < records.values.size(); ++i) {
        query.prepare("INSERT INTO NodeDictData(dictID,fieldID,fidx,gidx) "
                      "VALUES(?,?,?,?)");
        query.addBindValue(dictID);
        query.addBindValue(fieldIDs.at(i));
        query.addBindValue(records.values.at(i).at(records.names.indexOf("fidx")));
        query.addBindValue(records.values.at(i).at(records.names.indexOf("gidx")));
        if (!query.exec())
            return false;
    }
    db->commit(query);
    emit dictInfoFound(info);

    qDebug() << "new dict" << info.value("name").toString();
    return true;
}
