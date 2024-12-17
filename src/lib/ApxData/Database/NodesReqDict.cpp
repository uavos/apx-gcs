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
#include "NodesReqDict.h"

using namespace db::nodes;

bool NodeSaveDict::run(QSqlQuery &query)
{
    if (!RequestNode::run(query))
        return false;

    if (!_nodeID)
        return false;

    // get hash
    QString hash;
    {
        QCryptographicHash h(QCryptographicHash::Sha1);
        getHash(h, _dict["fields"]);
        hash = h.result().toHex().toUpper();
    }

    auto time = _dict["time"].toVariant().toULongLong();
    auto cache = _dict["cache"].toVariant();
    if (!cache.isNull() && cache.toString().isEmpty())
        cache = {};

    auto name = query.value("name");
    auto version = query.value("version");
    auto hardware = query.value("hardware");

    // find existing dictionary by node id and content hash
    query.prepare("SELECT * FROM NodeDict"
                  " WHERE nodeID=? AND hash=?"
                  " ORDER BY time DESC, key DESC"
                  " LIMIT 1");
    query.addBindValue(_nodeID);
    query.addBindValue(hash);
    if (!query.exec())
        return false;

    if (query.next()) {
        // dictionary exists
        qDebug() << "dict exists";
        auto dictID = query.value(0).toULongLong();
        if (cache.isNull())
            cache = query.value("cache");

        query.prepare("UPDATE NodeDict"
                      " SET time=?, name=?, version=?, hardware=?, cache=?"
                      " WHERE key=?");
        query.addBindValue(time);
        query.addBindValue(name);
        query.addBindValue(version);
        query.addBindValue(hardware);
        query.addBindValue(cache);
        query.addBindValue(dictID);
        if (!query.exec())
            return false;

        _dictID = dictID;
        emit dictSaved(dictID);
        return true;
    }

    // create new dict record
    db->transaction(query);
    query.prepare("INSERT INTO"
                  " NodeDict(nodeID,time,hash,name,version,hardware,cache)"
                  " VALUES(?,?,?,?,?,?,?)");
    query.addBindValue(_nodeID);
    query.addBindValue(time);
    query.addBindValue(hash);
    query.addBindValue(name);
    query.addBindValue(version);
    query.addBindValue(hardware);
    query.addBindValue(cache);
    if (!query.exec())
        return false;

    auto dictID = query.lastInsertId().toULongLong();

    // update fields records
    const auto columns = QStringList() << "name"
                                       << "title"
                                       << "units"
                                       << "type"
                                       << "array";
    uint add_cnt = 0;
    QList<quint64> fieldIDs;
    for (const auto &i : _dict["fields"].toArray()) {
        const auto jso = i.toObject();

        // find existing field
        QVariantList rec_bind;
        QStringList st;
        for (auto k : columns) {
            QVariant v = jso.value(k).toVariant();
            if (v.isNull()) {
                st << QString("%1 IS NULL").arg(k);
            } else {
                st << QString("%1=?").arg(k);
                rec_bind.append(v);
            }
        }
        query.prepare("SELECT key FROM NodeField "
                      " WHERE "
                      + st.join(" AND "));
        for (auto i : rec_bind)
            query.addBindValue(i);
        if (!query.exec())
            return false;
        if (query.next()) {
            fieldIDs.append(query.value(0).toULongLong());
            continue;
        }

        // create a new field
        query.prepare("INSERT INTO NodeField(" + columns.join(',')
                      + ")"
                        " VALUES(?,?,?,?,?)");
        for (auto k : columns) {
            query.addBindValue(jso.value(k).toVariant());
        }
        if (!query.exec())
            return false;
        //qDebug()<<"new field"<<vlist.at(0).toString();
        fieldIDs.append(query.lastInsertId().toULongLong());
        add_cnt++;
    }
    if (add_cnt > 0)
        qDebug() << "new fields:" << add_cnt;

    // write dict fields
    for (uint i = 0; i < fieldIDs.size(); ++i) {
        query.prepare("INSERT INTO NodeDictStruct(dictID,fieldID,fieldIndex) "
                      "VALUES(?,?,?)");
        query.addBindValue(dictID);
        query.addBindValue(fieldIDs.at(i));
        query.addBindValue(i);
        if (!query.exec())
            return false;
    }
    db->commit(query);

    qDebug() << "new dict" << name.toString() << version.toString() << hardware.toString() << hash
             << cache.toString();

    _dictID = dictID;
    emit dictSaved(dictID);
    return true;
}

bool NodeLoadDict::run(QSqlQuery &query)
{
    if (!_dictID) {
        // cache lookup mode
        if (!RequestNode::run(query))
            return false;

        if (!_nodeID) {
            qWarning() << "no node id";
            return false;
        }

        if (_cache_hash.isEmpty()) {
            qWarning() << "no cache hash";
            return false;
        }

        query.prepare("SELECT * FROM NodeDict"
                      " WHERE nodeID=? AND cache=?"
                      " ORDER BY time DESC, key DESC"
                      " LIMIT 1");
        query.addBindValue(_nodeID);
        query.addBindValue(_cache_hash);

        if (!query.exec())
            return false;

        if (!query.next()) {
            qDebug() << "no dict in db";
            emit dictMissing(_cache_hash);
            return true;
        }
        _dictID = query.value(0).toULongLong();
    } else {
        query.prepare("SELECT * FROM NodeDict WHERE key=?");
        query.addBindValue(_dictID);

        if (!query.exec())
            return false;

        if (!query.next()) {
            qDebug() << "no dict in db by key" << _dictID;
            return true;
        }
        _cache_hash = query.value("cache").toString();
    }
    auto time = query.value("time").toULongLong();

    // read dict fields
    query.prepare("SELECT * FROM NodeDictStruct "
                  "INNER JOIN NodeField ON NodeDictStruct.fieldID=NodeField.key "
                  "WHERE dictID=? "
                  "ORDER BY fieldIndex ASC");
    query.addBindValue(_dictID);
    if (!query.exec())
        return false;

    QJsonArray fields;
    while (query.next()) {
        auto field = record_to_json(query.record(),
                                    {
                                        "name",
                                        "title",
                                        "type",
                                        "array",
                                        "units",
                                    });

        field = json::fix_numbers(json::remove_empty(field));
        fields.append(field);
    }

    _dict["fields"] = fields;
    _dict["time"] = (qint64) time;

    if (!_cache_hash.isEmpty())
        _dict["cache"] = _cache_hash;

    emit dictLoaded(_dictID, _dict);
    return true;
}
