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
#include "VehiclesReqNode.h"

bool DBReqNode::run(QSqlQuery &query)
{
    if (!nodeID) {
        query.prepare("SELECT * FROM Nodes WHERE sn=?");
        query.addBindValue(_uid);
        if (!query.exec())
            return false;
        if (query.next()) {
            nodeID = query.value(0).toULongLong();
            emit foundID(nodeID);
        }
    } else {
        query.prepare("SELECT * FROM Nodes WHERE key=?");
        query.addBindValue(nodeID);
        if (!query.exec())
            return false;
        if (query.next()) {
            _uid = query.value("sn").toString();
        }
    }
    return true;
}

bool DBReqSaveNodeInfo::run(QSqlQuery &query)
{
    if (!DBReqNode::run(query))
        return false;

    auto time = _info.value("time").toULongLong();
    auto name = _info.value("name").toString();

    if (!nodeID) {
        //register node
        query.prepare("INSERT INTO Nodes(sn) VALUES(?)");
        query.addBindValue(_uid);
        if (!query.exec())
            return false;
        nodeID = query.lastInsertId().toULongLong();
        qDebug() << "new node" << name;
        emit foundID(nodeID);
    } else {
        // check for imported node in the past vs recently seen
        if (time <= query.value("time").toULongLong()) {
            qDebug() << "node import skipped" << name;
            return true;
        }
    }

    //uptime node info
    query.prepare("UPDATE Nodes SET time=?, name=?, version=?, hardware=? WHERE key=?");
    query.addBindValue(time ? time : QVariant());
    query.addBindValue(name);
    query.addBindValue(_info.value("version"));
    query.addBindValue(_info.value("hardware"));
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;

    // update node user
    if (!time)
        return true;

    auto user = _info.value("user").value<QVariantMap>();

    // find existing record
    query.prepare("SELECT * FROM NodeUsers WHERE nodeID=?");
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    if (query.next()) {
        //update record
        auto key = query.value(0).toULongLong();
        if (time <= query.value("time").toULongLong())
            return true; //imported

        query.prepare("UPDATE NodeUsers"
                      " SET time=?,machineUID=?,hostname=?,username=?"
                      " WHERE key=?");
        query.addBindValue(time);
        query.addBindValue(user.value("machineUID"));
        query.addBindValue(user.value("hostname"));
        query.addBindValue(user.value("username"));
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
    query.addBindValue(time);
    query.addBindValue(time);
    query.addBindValue(user.value("machineUID"));
    query.addBindValue(user.value("hostname"));
    query.addBindValue(user.value("username"));
    if (!query.exec())
        return false;
    qDebug() << "new node user" << user.value("username").toString()
             << user.value("hostname").toString();

    return true;
}

bool DBReqLoadNodeInfo::run(QSqlQuery &query)
{
    if (!DBReqNode::run(query))
        return false;

    if (!nodeID)
        return true;

    _info.insert("uid", _uid);
    _info.insert("time", query.value("time"));
    _info.insert("name", query.value("name"));
    _info.insert("version", query.value("version"));
    _info.insert("hardware", query.value("hardware"));

    // read user
    query.prepare("SELECT * FROM NodeUsers WHERE nodeID=?");
    query.addBindValue(nodeID);
    if (!query.exec())
        return false;
    if (query.next()) {
        QVariantMap user;
        user.insert("machineUID", query.value("machineUID"));
        user.insert("hostname", query.value("hostname"));
        user.insert("username", query.value("username"));
        _info.insert("user", user);
    }

    emit infoLoaded(_info);
    return true;
}

bool DBReqSaveNodeDict::run(QSqlQuery &query)
{
    if (!DBReqNode::run(query))
        return false;

    if (!nodeID)
        return false;

    auto hash = _dict.value("hash").toString();

    auto time = _dict.value("time").toULongLong();
    auto name = query.value("name");
    auto version = query.value("version");
    auto hardware = query.value("hardware");

    //find existing dictionary
    query.prepare(
        "SELECT * FROM NodeDicts WHERE nodeID=? AND hash=? ORDER BY time DESC, key DESC LIMIT 1");
    query.addBindValue(nodeID);
    query.addBindValue(hash);
    if (!query.exec())
        return false;

    if (query.next()) {
        //dictionary exists
        qDebug() << "dict exists";
        auto dictID = query.value(0).toULongLong();

        query.prepare("UPDATE NodeDicts SET time=?, name=?, version=?, hardware=? WHERE key=?");
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
    query.prepare("INSERT INTO NodeDicts(nodeID,time,hash,name,version,hardware) "
                  "VALUES(?,?,?,?,?,?)");
    query.addBindValue(nodeID);
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
    auto fields = _dict.value("fields").value<QVariantList>();
    for (auto i : fields) {
        auto m = filterNullValues(i.value<QVariantMap>());
        // find existing field
        QVariantList values;
        QStringList st;
        for (auto k : columns) {
            QVariant v = m.value(k);
            if (v.isNull()) {
                st << QString("%1 IS NULL").arg(k);
            } else {
                st << QString("%1=?").arg(k);
                values.append(v);
            }
        }
        query.prepare("SELECT key FROM NodeDictDataFields "
                      " WHERE "
                      + st.join(" AND "));
        for (auto i : values)
            query.addBindValue(i);
        if (!query.exec())
            return false;
        if (query.next()) {
            fieldIDs.append(query.value(0).toULongLong());
            continue;
        }
        // create a new field
        query.prepare("INSERT INTO NodeDictDataFields(" + columns.join(',')
                      + ") "
                        "VALUES(?,?,?,?,?)");
        for (auto k : columns) {
            query.addBindValue(m.value(k));
        }
        if (!query.exec())
            return false;
        //qDebug()<<"new field"<<vlist.at(0).toString();
        fieldIDs.append(query.lastInsertId().toULongLong());
        qDebug() << "new field" << m.value("name").toString();
    }

    //write dict fields
    for (uint i = 0; i < fieldIDs.size(); ++i) {
        query.prepare("INSERT INTO NodeDictData(dictID,fieldID,fidx) "
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

bool DBReqLoadNodeDict::run(QSqlQuery &query)
{
    if (!_dictID) {
        if (!DBReqNode::run(query))
            return false;

        if (!nodeID)
            return false;

        query.prepare("SELECT * FROM NodeDicts "
                      "WHERE nodeID=? AND hash=? "
                      "ORDER BY time DESC, key DESC "
                      "LIMIT 1");
        query.addBindValue(nodeID);
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
        query.prepare("SELECT * FROM NodeDicts WHERE key=?");
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
    query.prepare("SELECT * FROM NodeDictData "
                  "INNER JOIN NodeDictDataFields ON NodeDictData.fieldID=NodeDictDataFields.key "
                  "WHERE dictID=? "
                  "ORDER BY fidx ASC");
    query.addBindValue(_dictID);
    if (!query.exec())
        return false;

    QVariantList fields;
    while (query.next()) {
        QVariantMap field;
        field.insert("name", query.value("name"));
        field.insert("title", query.value("title"));
        field.insert("type", query.value("type"));
        if (!query.value("array").isNull())
            field.insert("array", query.value("array"));
        if (!query.value("units").isNull())
            field.insert("units", query.value("units"));
        else if (query.value("type").toString() == "option")
            field.insert("units", QString("off,on"));

        fields.append(field);
    }

    _dict.insert("hash", _hash);
    _dict.insert("fields", fields);
    _dict.insert("time", time);
    _dict.insert("cached", true);

    emit dictLoaded(_dict);
    return true;
}

bool DBReqSaveNodeConfig::run(QSqlQuery &query)
{
    if (!DBReqNode::run(query))
        return false;

    if (!nodeID) {
        qWarning() << "no node in db";
        return false;
    }

    query.prepare("SELECT * FROM NodeDicts "
                  "WHERE nodeID=? AND hash=? "
                  "ORDER BY time DESC, key DESC "
                  "LIMIT 1");
    query.addBindValue(nodeID);
    query.addBindValue(_hash);

    if (!query.exec())
        return false;

    if (!query.next()) {
        qWarning() << "no dict in db";
        return true;
    }
    auto dictID = query.value(0).toULongLong();

    //grab title from node's comment
    auto title = _values.value("label").toString();
    title = title.simplified().trimmed();

    //generate hash
    QCryptographicHash h(QCryptographicHash::Sha1);
    h.addData(_uid.toUtf8());
    h.addData(_hash.toUtf8());
    getHash(h, _values);
    QString hash = h.result().toHex().toUpper();

    //find existing config
    query.prepare("SELECT * FROM NodeConfigs"
                  " WHERE hash=?");
    query.addBindValue(hash);
    if (!query.exec())
        return false;

    if (query.next()) {
        _configID = query.value(0).toULongLong();
        //same node config exist - uptime
        //qDebug()<<"node config exists";
        //find latest config in history
        query.prepare("SELECT * FROM NodeConfigHistory"
                      " WHERE nconfID=? AND time<=?"
                      " ORDER BY time DESC LIMIT 1");
        query.addBindValue(_configID);
        query.addBindValue(_time);
        if (!query.exec())
            return false;

        if (query.next()) {
            //same data config exists and is latest
            //uptime time
            quint64 nconfHistoryID = query.value(0).toULongLong();
            //uptime config history record with actual data
            query.prepare("UPDATE NodeConfigHistory SET time=? WHERE key=?");
            query.addBindValue(_time);
            query.addBindValue(nconfHistoryID);
            if (!query.exec())
                return false;
            qDebug() << "config exists" << title;
            //all ok
            emit dbModified();
            emit configSaved(_configID);
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
        query.addBindValue(_time);
        query.addBindValue(hash);
        query.addBindValue(title.isEmpty() ? QVariant() : title);
        if (!query.exec())
            return false;

        _configID = query.lastInsertId().toULongLong();

        //collect field IDs
        QHash<QString, quint64> fieldsMap;
        query.prepare(
            "SELECT * FROM NodeDictData "
            "INNER JOIN NodeDictDataFields ON NodeDictData.fieldID=NodeDictDataFields.key "
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
        for (auto const &key : _values.keys()) {
            quint64 fieldID = fieldsMap.value(key, 0);
            if (!fieldID) {
                qWarning() << "missing field" << key;
                return false;
            }
            const QVariant &v = _values.value(key);
            if (static_cast<QMetaType::Type>(v.type()) == QMetaType::QVariantList) {
                int subidx = 0;
                for (auto const &i : v.value<QVariantList>()) {
                    quint64 valueID = getValueID(query, i);
                    if (!valueID)
                        return false;

                    //insert new record
                    query.prepare("INSERT INTO NodeConfigData(nconfID,fieldID,subidx,valueID) "
                                  "VALUES(?,?,?,?)");
                    query.addBindValue(_configID);
                    query.addBindValue(fieldID);
                    query.addBindValue(subidx);
                    query.addBindValue(valueID);
                    if (!query.exec())
                        return false;
                    dcnt++;
                    subidx++;
                }
            } else {
                //find valueID
                quint64 valueID = getValueID(query, v);
                if (!valueID)
                    return false;

                //insert new record
                query.prepare("INSERT INTO NodeConfigData(nconfID,fieldID,valueID) VALUES(?,?,?)");
                query.addBindValue(_configID);
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
        qDebug() << "node config created" << title;
    }

    //create new node config history record
    query.prepare("INSERT INTO NodeConfigHistory(nconfID,time) VALUES(?,?)");
    query.addBindValue(_configID);
    query.addBindValue(_time);
    if (!query.exec())
        return false;

    db->commit(query);
    emit dbModified();

    qDebug() << "node config updated" << title;

    emit configSaved(_configID);
    return true;
}
quint64 DBReqSaveNodeConfig::getValueID(QSqlQuery &query, const QVariant &v)
{
    if (v.isNull()) {
        query.prepare("SELECT key FROM NodeConfigDataValues WHERE value IS NULL LIMIT 1");
    } else {
        query.prepare("SELECT key FROM NodeConfigDataValues WHERE value=? LIMIT 1");
        query.addBindValue(v);
    }
    if (!query.exec())
        return 0;
    if (query.next())
        return query.value(0).toULongLong();

    query.prepare("INSERT INTO NodeConfigDataValues(value) VALUES(?)");
    query.addBindValue(v);
    if (!query.exec())
        return 0;
    return query.lastInsertId().toULongLong();
}

bool DBReqLoadNodeConfig::run(QSqlQuery &query)
{
    if (!_configID) {
        if (!DBReqNode::run(query))
            return false;

        if (!nodeID)
            return false;

        if (_hash.isEmpty()) {
            // load latest
            query.prepare("SELECT * FROM NodeConfigs WHERE nodeID = ? ORDER BY time DESC LIMIT 1");
            query.addBindValue(nodeID);
        } else {
            query.prepare("SELECT * FROM NodeConfigs"
                          " WHERE nodeID=? AND hash=?");
            query.addBindValue(nodeID);
            query.addBindValue(_hash);
        }
        if (!query.exec())
            return false;
        if (!query.next())
            return _hash.isEmpty();

        _configID = query.value(0).toULongLong();
    } else {
        query.prepare("SELECT * FROM NodeConfigs WHERE key=?");
        query.addBindValue(_configID);

        if (!query.exec())
            return false;
        if (!query.next()) {
            qWarning() << "configID not found" << _configID;
            return false;
        }
    }

    _time = query.value("time").toULongLong();

    //build values table
    query.prepare(
        "SELECT name, subidx, value FROM NodeConfigData"
        " INNER JOIN NodeDictData ON NodeConfigData.fieldID=NodeDictData.key"
        " INNER JOIN NodeDictDataFields ON NodeDictData.fieldID=NodeDictDataFields.key"
        " INNER JOIN NodeConfigDataValues ON NodeConfigData.valueID=NodeConfigDataValues.key"
        " WHERE NodeConfigData.nconfID=? ORDER BY name,subidx");
    query.addBindValue(_configID);
    if (!query.exec())
        return false;

    while (query.next()) {
        QString s = query.value(0).toString();
        if (_values.contains(s) && _values.value(s).typeId() != QMetaType::QVariantList) {
            qWarning() << "duplicate field" << s;
        }
        const QVariant &v = query.value(2);
        if (query.value(1).isNull()) {
            _values.insert(s, v);
            continue;
        }
        QVariantList list = _values.contains(s) ? _values.value(s).value<QVariantList>()
                                                : QVariantList();
        list.append(v);
        _values.insert(s, list);
    }
    emit configLoaded(_values);
    return true;
}

bool DBReqSaveNodeMeta::run(QSqlQuery &query)
{
    uint tcnt = 0, ncnt = 0, ucnt = 0;
    for (auto name : _meta.keys()) {
        tcnt++;
        query.prepare("SELECT * FROM NodeDictMeta WHERE name=?");
        query.addBindValue(name);
        if (!query.exec())
            return false;

        auto m = _meta.value(name).value<QVariantMap>();
        auto version = m.value("version").toString();
        auto descr = m.value("descr").toString();
        auto def = m.value("def").toString();
        auto min = m.value("min").toString();
        auto max = m.value("max").toString();
        auto increment = m.value("increment").toString();
        auto decimal = m.value("decimal").toString();

        if (!query.next()) {
            query.prepare(
                "INSERT INTO NodeDictMeta(name,version,descr,def,min,max,increment,decimal) "
                "VALUES(?,?,?,?,?,?,?,?)");
            query.addBindValue(name);
            query.addBindValue(version);
            query.addBindValue(descr);
            query.addBindValue(def);
            query.addBindValue(min);
            query.addBindValue(max);
            query.addBindValue(increment);
            query.addBindValue(decimal);
            if (!query.exec())
                return false;
            ncnt++;
            continue;
        }

        // update existing
        auto qversion = query.value("version").toString();
        auto qdescr = query.value("descr").toString();
        auto qdef = query.value("def").toString();
        auto qmin = query.value("min").toString();
        auto qmax = query.value("max").toString();
        auto qincrement = query.value("increment").toString();
        auto qdecimal = query.value("decimal").toString();

        if (version.isEmpty())
            version = qversion;

        bool older = QVersionNumber::fromString(version) < QVersionNumber::fromString(qversion);

        if (older)
            version = qversion;

        if (descr.isEmpty() || older)
            descr = qdescr;

        if (older) {
            def = qdef;
            min = qmin;
            max = qmax;
            increment = qincrement;
            decimal = qdecimal;
        }

        // update record
        if (qversion == version && qdescr == descr && qdef == def && qmin == min && qmax == max
            && qincrement == increment && qdecimal == decimal) {
            continue; // not changed
        }

        auto key = query.value(0).toULongLong();
        query.prepare("UPDATE NodeDictMeta SET version=?, descr=?, def=?, min=?, max=?, "
                      "increment=?, decimal=? WHERE key=?");
        query.addBindValue(version);
        query.addBindValue(descr);
        query.addBindValue(def);
        query.addBindValue(min);
        query.addBindValue(max);
        query.addBindValue(increment);
        query.addBindValue(decimal);
        query.addBindValue(key);
        if (!query.exec())
            return false;
        ucnt++;
    }

    if (ncnt)
        qDebug() << "new meta:" << ncnt << "of" << tcnt;
    if (ucnt)
        qDebug() << "updated meta:" << ucnt << "of" << tcnt;

    return true;
}
bool DBReqLoadNodeMeta::run(QSqlQuery &query)
{
    for (auto name : _names) {
        query.prepare("SELECT * FROM NodeDictMeta WHERE name=?");
        query.addBindValue(name);
        if (!query.exec())
            return false;
        if (!query.next())
            continue;

        QVariantMap m;
        m.insert("descr", query.value("descr"));
        m.insert("def", query.value("def"));
        m.insert("min", query.value("min"));
        m.insert("max", query.value("max"));
        m.insert("increment", query.value("increment"));
        m.insert("decimal", query.value("decimal"));

        _meta.insert(name, m);
    }

    if (!_meta.isEmpty())
        emit metaDataLoaded(_meta);

    return true;
}
