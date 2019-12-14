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
#include "NodesReqNconf.h"
//=============================================================================
bool DBReqNodesLoadNconf::run(QSqlQuery &query)
{
    //read time to filter
    query.prepare("SELECT * FROM NodeConfigs"
                  " INNER JOIN Nodes ON NodeConfigs.nodeID=Nodes.key"
                  " WHERE NodeConfigs.key=?");
    query.addBindValue(nconfID);
    if (!query.exec())
        return false;
    if (!query.next())
        return false;

    info = queryRecord(query);

    //build values table
    query.prepare(
        "SELECT name, value FROM NodeConfigData"
        " INNER JOIN NodeDictData ON NodeConfigData.fieldID=NodeDictData.key"
        " INNER JOIN NodeDictDataFields ON NodeDictData.fieldID=NodeDictDataFields.key"
        " INNER JOIN NodeConfigDataValues ON NodeConfigData.valueID=NodeConfigDataValues.key"
        " WHERE NodeConfigData.nconfID=?");
    query.addBindValue(nconfID);
    if (!query.exec())
        return false;

    while (query.next()) {
        QString s = query.value(0).toString();
        if (values.contains(s)) {
            qWarning() << "duplicate field" << s;
        }
        values.insert(s, query.value(1));
    }
    emit nconfFound(nconfID);
    emit configLoaded(info, values);
    return true;
}
//=============================================================================
bool DBReqNodesLoadNconfLatest::run(QSqlQuery &query)
{
    if (!DBReqNodes::run(query))
        return false; //get nodeID
    if (!nodeID)
        return true;

    //find total
    query.prepare("SELECT * FROM NodeConfigs WHERE nodeID = ? ORDER BY time DESC LIMIT 1");
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    if (!query.next())
        return true;
    nconfID = query.value("key").toULongLong();
    return DBReqNodesLoadNconf::run(query);
}
//=============================================================================
bool DBReqNodesSaveNconf::run(QSqlQuery &query)
{
    if (dictInfo.contains("nodeID"))
        nodeID = dictInfo.value("nodeID").toULongLong();
    else if (!DBReqNodes::run(query))
        return false; //get nodeID
    if (!nodeID)
        return false;

    quint64 dictID = dictInfo.value("key").toULongLong();
    if (dictID) {
        //find dictID hash
        query.prepare("SELECT * FROM NodeDicts WHERE key=?");
        query.addBindValue(dictID);
        if (!query.exec())
            return false;
        if (!query.next())
            return false;
        dictInfo = queryRecord(query);
    } else {
        if (dictInfo.contains("hash")) {
            //find dictID by hash
            query.prepare("SELECT * FROM NodeDicts WHERE nodeID=? AND hash=? ORDER BY time DESC, "
                          "key DESC LIMIT 1");
            query.addBindValue(nodeID);
            query.addBindValue(dictInfo.value("hash").toString());
            if (!query.exec())
                return false;
            if (query.next()) {
                dictID = query.value("key").toULongLong();
            }
        }
    }
    if (!dictID) {
        qWarning() << "missing dictID" << dictInfo.keys();
        return true;
    }

    //generate hash
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(sn.toUtf8());
    h.addData(dictInfo.value("hash").toString().toUtf8());
    getHash(h, values);
    QString hash = h.result().toHex().toUpper();

    //grab title from node's comment
    QString title = values.value("name").toString();
    if (title.isEmpty())
        title = values.value("comment", values.value("node_label")).toString();
    title = title.simplified().trimmed();

    //find existing config
    query.prepare("SELECT * FROM NodeConfigs"
                  " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;
    if (query.next()) {
        nconfID = query.value(0).toULongLong();
        //same node config exist - uptime
        //qDebug()<<"node config exists";
        //find latest config in history
        query.prepare("SELECT * FROM NodeConfigHistory"
                      " WHERE nconfID=? AND time<=?"
                      " ORDER BY time DESC LIMIT 1");
        query.addBindValue(nconfID);
        query.addBindValue(time);
        if (!query.exec())
            return false;
        if (query.next()) {
            //same data config exists and is latest
            //uptime time
            quint64 nconfHistoryID = query.value(0).toULongLong();
            //uptime config history record with actual data
            query.prepare("UPDATE NodeConfigHistory SET time=? WHERE key=?");
            query.addBindValue(time);
            query.addBindValue(nconfHistoryID);
            if (!query.exec())
                return false;
            //all ok
            emit dbModified();
            emit nconfFound(nconfID);
            return true;
        }
        //continue to new history record
        db->transaction(query);
    } else {
        //config not exists - create new entry
        db->transaction(query);

        query.prepare("INSERT INTO NodeConfigs("
                      "nodeID, dictID, time, hash, title"
                      ") VALUES(?, ?, ?, ?, ?)");
        query.addBindValue(nodeID);
        query.addBindValue(dictID);
        query.addBindValue(time);
        query.addBindValue(hash);
        query.addBindValue(title.isEmpty() ? QVariant() : title);
        if (!query.exec())
            return false;

        nconfID = query.lastInsertId().toULongLong();

        //collect field IDs
        QHash<QString, quint64> fieldsMap = getFieldsByName(query, dictID);

        //write values
        int dcnt = 0;
        foreach (QString s, values.keys()) {
            quint64 fieldID = fieldsMap.value(s, 0);
            if (!fieldID) {
                qWarning() << "missing field" << s;
                return false;
            }
            //find valueID
            quint64 valueID = 0;
            QVariant v = values.value(s);
            if (v.isNull()) {
                query.prepare("SELECT key FROM NodeConfigDataValues WHERE value IS NULL LIMIT 1");
            } else {
                query.prepare("SELECT key FROM NodeConfigDataValues WHERE value=? LIMIT 1");
                query.addBindValue(v);
            }
            if (!query.exec())
                return false;
            if (query.next())
                valueID = query.value(0).toULongLong();
            else {
                query.prepare("INSERT INTO NodeConfigDataValues(value) VALUES(?)");
                query.addBindValue(v);
                if (!query.exec())
                    return false;
                valueID = query.lastInsertId().toULongLong();
            }
            //insert new record
            query.prepare("INSERT INTO NodeConfigData(nconfID,fieldID,valueID) VALUES(?,?,?)");
            query.addBindValue(nconfID);
            query.addBindValue(fieldID);
            query.addBindValue(valueID);
            if (!query.exec())
                return false;
            dcnt++;
        }
        if (dcnt <= 0) {
            qWarning() << "nothing to save";
            return false;
        }
        qDebug() << "node config created" << title;
    }

    //create new node config history record
    query.prepare("INSERT INTO NodeConfigHistory(nconfID,time) VALUES(?,?)");
    query.addBindValue(nconfID);
    query.addBindValue(time);
    if (!query.exec())
        return false;

    db->commit(query);
    emit dbModified();
    emit nconfFound(nconfID);

    qDebug() << "node config updated" << title;
    return true;
}
//=============================================================================
