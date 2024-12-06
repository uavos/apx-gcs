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

    auto hash = _dict["hash"].toString();
    auto time = _dict["time"].toInteger();

    auto name = query.value("name");
    auto version = query.value("version");
    auto hardware = query.value("hardware");

    //find existing dictionary
    query.prepare("SELECT * FROM NodeDict"
                  " WHERE nodeID=? AND hash=?"
                  " ORDER BY time DESC, key DESC"
                  " LIMIT 1");
    query.addBindValue(_nodeID);
    query.addBindValue(hash);
    if (!query.exec())
        return false;

    if (query.next()) {
        //dictionary exists
        qDebug() << "dict exists";
        auto dictID = query.value(0).toULongLong();

        query.prepare("UPDATE NodeDict"
                      " SET time=?, name=?, version=?, hardware=?"
                      " WHERE key=?");
        query.addBindValue(time);
        query.addBindValue(name);
        query.addBindValue(version);
        query.addBindValue(hardware);
        query.addBindValue(dictID);
        if (!query.exec())
            return false;

        return true;
    }

    // create new dict record
    db->transaction(query);
    query.prepare("INSERT INTO"
                  " NodeDict(nodeID,time,hash,name,version,hardware)"
                  " VALUES(?,?,?,?,?,?)");
    query.addBindValue(_nodeID);
    query.addBindValue(time);
    query.addBindValue(hash);
    query.addBindValue(name);
    query.addBindValue(version);
    query.addBindValue(hardware);
    if (!query.exec())
        return false;

    auto dictID = query.lastInsertId().toULongLong();

    // update fields records
    const auto columns = QStringList() << "name"
                                       << "title"
                                       << "units"
                                       << "type"
                                       << "array";
    QList<quint64> fieldIDs;
    for (const auto &i : _dict["fields"].toArray()) {
        const auto m = json::filter_names(i.toObject());

        // find existing field
        QVariantList rec_bind;
        QStringList st;
        for (auto k : columns) {
            QVariant v = m[k];
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
            query.addBindValue(m.value(k));
        }
        if (!query.exec())
            return false;
        //qDebug()<<"new field"<<vlist.at(0).toString();
        fieldIDs.append(query.lastInsertId().toULongLong());
        qDebug() << "new field" << m["name"].toString();
    }

    //write dict fields
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

    qDebug() << "new dict" << name.toString() << version.toString() << hardware.toString() << hash;
    return true;
}

bool NodeLoadDict::run(QSqlQuery &query)
{
    if (!_dictID) {
        if (!RequestNode::run(query))
            return false;

        if (!_nodeID)
            return false;

        query.prepare("SELECT * FROM NodeDict"
                      " WHERE nodeID=? AND hash=?"
                      " ORDER BY time DESC, key DESC"
                      " LIMIT 1");
        query.addBindValue(_nodeID);
        query.addBindValue(_hash);

        if (!query.exec())
            return false;

        if (!query.next()) {
            qDebug() << "no dict in db";
            emit dictMissing(_hash);
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
        _hash = query.value("hash").toString();
    }
    auto time = query.value("time").toULongLong();

    //read fields
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

        // if (query.value("type").toString() == "option" && field.value("units").isNull())
        //     field["units"] = QString("off,on");

        field = json::fix_numbers(json::filter_names(field));
        fields.append(field);
    }

    _dict["hash"] = _hash;
    _dict["fields"] = fields;
    _dict["time"] = (qint64) time;
    _dict["cached"] = true;

    emit dictLoaded(_dict);
    return true;
}
