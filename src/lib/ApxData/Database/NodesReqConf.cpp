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
#include "NodesReqConf.h"

using namespace db::nodes;

bool NodeSaveConf::run(QSqlQuery &query)
{
    if (!RequestNode::run(query))
        return false;

    if (!_nodeID) {
        qWarning() << "no node in db";
        return false;
    }

    query.prepare("SELECT * FROM NodeDict"
                  " WHERE nodeID=? AND hash=?"
                  " ORDER BY time DESC, key DESC"
                  " LIMIT 1");
    query.addBindValue(_nodeID);
    query.addBindValue(_hash);

    if (!query.exec())
        return false;

    if (!query.next()) {
        qWarning() << "no dict in db";
        return true;
    }
    auto dictID = query.value(0).toULongLong();

    //grab title from node's comment
    auto title = _values["label"].toString().simplified().trimmed();

    //generate hash
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(_uid.toUtf8());
    h.addData(_hash.toUtf8());
    getHash(h, _values);
    QString hash = h.result().toHex().toUpper();

    //find existing conf
    query.prepare("SELECT * FROM NodeConf"
                  " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;

    if (query.next()) { //same node conf exist
        _nodeConfID = query.value(0).toULongLong();

        // update conf record with actual time
        query.prepare("UPDATE NodeConf SET time=? WHERE key=?");
        query.addBindValue(_time);
        query.addBindValue(_nodeConfID);
        if (!query.exec())
            return false;
        qDebug() << "conf exists" << title;
        //all ok
        emit dbModified();
        emit confSaved(_nodeConfID);
        return true;
    }

    //conf not exists - create new entry
    db->transaction(query);

    query.prepare("INSERT INTO NodeConf("
                  " nodeID, dictID, time, hash, title"
                  " ) VALUES(?, ?, ?, ?, ?)");
    query.addBindValue(_nodeID);
    query.addBindValue(dictID);
    query.addBindValue(_time);
    query.addBindValue(hash);
    query.addBindValue(title.isEmpty() ? QVariant() : title);
    if (!query.exec())
        return false;

    _nodeConfID = query.lastInsertId().toULongLong();

    //collect field IDs
    QHash<QString, quint64> fieldsMap;
    query.prepare("SELECT * FROM NodeDictStruct "
                  "INNER JOIN NodeField ON NodeDictStruct.fieldID=NodeField.key "
                  "WHERE dictID=? AND type!='command' AND type!='group'");
    query.addBindValue(dictID);
    if (!query.exec())
        return false;

    while (query.next()) {
        QString s = query.value("name").toString();
        if (fieldsMap.contains(s)) {
            qWarning() << "duplicate field name";
            fieldsMap.clear();
            break;
        }
        fieldsMap.insert(s, query.value(0).toULongLong());
    }
    if (fieldsMap.isEmpty()) {
        qWarning() << "missing fields in dictionary" << dictID;
        return true;
    }

    //write values
    int dcnt = 0;
    for (auto it = _values.begin(); it != _values.end(); ++it) {
        const auto &key = it.key();
        const auto &value = it.value();

        quint64 fieldID = fieldsMap.value(key, 0);
        if (!fieldID) {
            qWarning() << "missing field" << key;
            return false;
        }
        if (value.isArray()) {
            int subidx = 0;
            for (const auto &jsv : value.toArray()) {
                if (jsv.isNull()) { // don't record null values
                    subidx++;
                    continue;
                }

                quint64 valueID = _getValueID(query, jsv);
                if (!valueID)
                    return false;

                //insert new record
                query.prepare("INSERT INTO NodeConfData(confID,fieldID,subidx,valueID) "
                              "VALUES(?,?,?,?)");
                query.addBindValue(_nodeConfID);
                query.addBindValue(fieldID);
                query.addBindValue(subidx);
                query.addBindValue(valueID);
                if (!query.exec())
                    return false;
                dcnt++;
                subidx++;
            }
        } else {
            if (value.isNull()) // don't record null values
                continue;

            //find valueID
            quint64 valueID = _getValueID(query, value);
            if (!valueID)
                return false;

            //insert new record
            query.prepare("INSERT INTO NodeConfData(confID,fieldID,valueID) VALUES(?,?,?)");
            query.addBindValue(_nodeConfID);
            query.addBindValue(fieldID);
            query.addBindValue(valueID);
            if (!query.exec())
                return false;
            dcnt++;
        }
    }
    if (dcnt <= 0) {
        qWarning() << "nothing to save" << title;
        return false;
    }
    qDebug() << "node conf created" << title;

    db->commit(query);
    emit dbModified();

    qDebug() << "node conf updated" << title;

    emit confSaved(_nodeConfID);
    return true;
}
quint64 NodeSaveConf::_getValueID(QSqlQuery &query, const QJsonValue &v)
{
    if (v.isNull()) {
        query.prepare("SELECT key FROM NodeConfValues WHERE value IS NULL LIMIT 1");
    } else {
        query.prepare("SELECT key FROM NodeConfValues WHERE value=? LIMIT 1");
        query.addBindValue(v.toVariant());
    }
    if (!query.exec())
        return 0;
    if (query.next())
        return query.value(0).toULongLong();

    query.prepare("INSERT INTO NodeConfValues(value) VALUES(?)");
    query.addBindValue(v.toVariant());
    if (!query.exec())
        return 0;
    return query.lastInsertId().toULongLong();
}

bool NodeLoadConf::run(QSqlQuery &query)
{
    // qDebug() << "load conf" << _nodeID << _hash << _nodeConfID;
    if (!_nodeConfID) {
        if (!RequestNode::run(query))
            return false;

        if (!_nodeID)
            return false;

        if (_hash.isEmpty()) {
            // load latest
            query.prepare("SELECT * FROM NodeConf"
                          " WHERE nodeID=?"
                          " ORDER BY time DESC LIMIT 1");
            query.addBindValue(_nodeID);
        } else {
            query.prepare("SELECT * FROM NodeConf"
                          " WHERE nodeID=? AND hash=?");
            query.addBindValue(_nodeID);
            query.addBindValue(_hash);
        }
        if (!query.exec())
            return false;
        if (!query.next())
            return _hash.isEmpty();

        _nodeConfID = query.value(0).toULongLong();
    } else {
        query.prepare("SELECT * FROM NodeConf WHERE key=?");
        query.addBindValue(_nodeConfID);

        if (!query.exec())
            return false;
        if (!query.next()) {
            qWarning() << "confID not found" << _nodeConfID;
            return false;
        }
    }

    _time = query.value("time").toULongLong();

    //build values table
    query.prepare("SELECT name, subidx, value FROM NodeConfData"
                  " INNER JOIN NodeDictStruct ON NodeConfData.fieldID=NodeDictStruct.key"
                  " INNER JOIN NodeField ON NodeDictStruct.fieldID=NodeField.key"
                  " INNER JOIN NodeConfValues ON NodeConfData.valueID=NodeConfValues.key"
                  " WHERE NodeConfData.confID=? ORDER BY name,subidx");
    query.addBindValue(_nodeConfID);
    if (!query.exec())
        return false;

    while (query.next()) {
        auto s = query.value(0).toString();
        if (_values.contains(s) && !_values.value(s).isArray()) {
            qWarning() << "duplicate field" << s;
        }
        const auto jsv = QJsonValue::fromVariant(query.value(2));
        if (jsv.isNull())
            continue;

        if (query.value(1).isNull()) { // no subidx (not array)
            _values[s] = jsv;
            continue;
        }
        // array
        auto subidx = query.value(1).toInt();
        if (subidx < 0 || subidx > 10000) {
            qWarning() << "subidx error" << subidx;
            continue;
        }
        auto jsa = _values[s].toArray();
        while (jsa.size() < subidx)
            jsa.append(QJsonValue());
        jsa.append(jsv);
        _values[s] = jsa;
    }

    _values = json::remove_empty(json::fix_numbers(_values), true);

    emit confLoaded(_values);
    return true;
}
