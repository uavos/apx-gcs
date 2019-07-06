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
#include "NodesReqDict.h"
//=============================================================================
bool DBReqNodesLoadInfo::run(QSqlQuery &query)
{
    if (!DBReqNodes::run(query))
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
//=============================================================================
bool DBReqNodesSaveInfo::run(QSqlQuery &query)
{
    if (!DBReqNodes::run(query))
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
//=============================================================================
bool DBReqNodesSaveUser::run(QSqlQuery &query)
{
    if (!DBReqNodes::run(query))
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
bool DBReqNodesLoadUser::run(QSqlQuery &query)
{
    if (!DBReqNodes::run(query))
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
//=============================================================================
//=============================================================================
bool DBReqNodesLoadDict::run(QSqlQuery &query)
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
        if (!DBReqNodes::run(query))
            return false; //get nodeID
        if (!nodeID)
            return true;
        if (!chash.isEmpty()) {
            //find existing dictionary by chash
            query.prepare("SELECT * FROM NodeDicts "
                          "INNER JOIN Nodes ON NodeDicts.nodeID=Nodes.key "
                          "WHERE nodeID=? AND chash=? "
                          "ORDER BY NodeDicts.time DESC, NodeDicts.key DESC "
                          "LIMIT 1");
            query.addBindValue(nodeID);
            query.addBindValue(chash);
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
        qWarning() << "missing node dict" << sn << chash;
        return true;
    }

    info = queryRecord(query);

    //read fields
    query.prepare("SELECT * FROM NodeDictData "
                  "INNER JOIN NodeDictDataFields ON NodeDictData.fieldID=NodeDictDataFields.key "
                  "WHERE dictID=? "
                  "ORDER BY id ASC, subID ASC");
    query.addBindValue(dictID);
    if (!query.exec())
        return false;
    while (query.next()) {
        const QString dtype = query.value("dtype").toString();
        quint16 id = static_cast<quint16>(query.value("id").toUInt());
        if (dtype == "command") {
            DictNode::Command c;
            c.cmd = id;
            c.name = query.value("name").toString();
            c.descr = query.value("descr").toString();
            dict.commands.append(c);
            continue;
        }
        QVariant vsubID = query.value("subID");
        if (vsubID.isNull() && id != dict.fields.size()) {
            qDebug() << "wrong params sequence" << id;
            return false;
        }
        DictNode::Field f;
        f.id = id;
        f.type = DictNode::dataTypeFromString(dtype);
        f.name = query.value("name").toString();
        f.title = query.value("title").toString();
        f.descr = query.value("descr").toString();
        f.units = query.value("units").toString();
        f.opts = query.value("opts").toString().split(',', QString::SkipEmptyParts);
        f.groups = query.value("sect").toString().split('/', QString::SkipEmptyParts);
        if (f.type == DictNode::Void) {
            qDebug() << "wrong field type" << f.name << f.type;
            return false;
        }
        f.valid = true;
        if (vsubID.isNull()) {
            dict.fields.append(f);
            continue;
        }
        int subID = vsubID.toInt();
        if (dict.fields.isEmpty()) {
            qDebug() << "wrong sub params sequence" << f.id;
        }
        DictNode::Field &fp = dict.fields[dict.fields.size() - 1];
        if (f.id != fp.id || fp.subFields.size() != subID) {
            qDebug() << "wrong sub params sequence" << subID << fp.id << f.id
                     << fp.subFields.size();
            return false;
        }
        f.id = static_cast<quint16>(subID);
        fp.subFields.append(f);
    }

    //valitime and publish dict
    dict.fieldsValid = true;
    dict.commandsValid = true;
    dict.chash = info.value("chash").toString();
    dict.cached = true;

    //qDebug()<<t0.elapsed()<<"ms";
    emit dictInfoFound(info);
    emit dictLoaded(info, dict);
    return true;
}
//=============================================================================
void DBReqNodesSaveDict::makeRecords(const DictNode::Dict &dict)
{
    records.names << "id"
                  << "subID"
                  << "name"
                  << "title"
                  << "descr"
                  << "units"
                  << "dtype"
                  << "opts"
                  << "sect";
    for (int i = 0; i < dict.fields.size(); ++i) {
        const DictNode::Field &f = dict.fields.at(i);
        records.values.append(QVariantList() << f.id << QVariant() << f.name << f.title << f.descr
                                             << f.units << DictNode::dataTypeToString(f.type)
                                             << f.opts.join(',') << f.groups.join('/'));
        for (int j = 0; j < f.subFields.size(); ++j) {
            const DictNode::Field &fs = f.subFields.at(j);
            records.values.append(QVariantList() << f.id << fs.id << fs.name << fs.title << fs.descr
                                                 << fs.units << DictNode::dataTypeToString(fs.type)
                                                 << fs.opts.join(',') << fs.groups.join('/'));
        }
    }
    for (int i = 0; i < dict.commands.size(); i++) {
        const DictNode::Command &c = dict.commands.at(i);
        records.values.append(QVariantList()
                              << c.cmd << QVariant() << c.name << QVariant() << c.descr
                              << QVariant() << QString("command") << QVariant() << QVariant());
    }
}
bool DBReqNodesSaveDict::run(QSqlQuery &query)
{
    if (info.contains("nodeID"))
        nodeID = info.value("nodeID").toULongLong();
    else if (!DBReqNodes::run(query))
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
            if (info.value("chash").toString().isEmpty())
                info.insert("chash", query.value("chash").toString());
            query.prepare("UPDATE NodeDicts SET time=?, chash=? WHERE key = ?");
            query.addBindValue(info.value("time").toULongLong());
            query.addBindValue(info.value("chash").toString());
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
    query.prepare("INSERT INTO NodeDicts(nodeID,time,hash,name,version,hardware,chash) "
                  "VALUES(?,?,?,?,?,?,?)");
    query.addBindValue(nodeID);
    query.addBindValue(info.value("time").toULongLong());
    query.addBindValue(info.value("hash").toString());
    query.addBindValue(info.value("name").toString());
    query.addBindValue(info.value("version").toString());
    query.addBindValue(info.value("hardware").toString());
    query.addBindValue(info.value("chash").toString());
    if (!query.exec())
        return false;
    dictID = query.lastInsertId().toULongLong();
    info["key"] = dictID;

    //uptime fields records
    QList<quint64> flist;
    QStringList fnames;
    fnames << "name"
           << "dtype"
           << "title"
           << "descr"
           << "units"
           << "opts"
           << "sect";
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
            flist.append(query.value(0).toULongLong());
            continue;
        }

        query.prepare("INSERT INTO NodeDictDataFields(" + fnames.join(',')
                      + ") "
                        "VALUES(?,?,?,?,?,?,?)");
        for (int j = 0; j < fnames.size(); ++j) {
            query.addBindValue(vlist.at(j));
        }
        if (!query.exec())
            return false;
        //qDebug()<<"new field"<<vlist.at(0).toString();
        flist.append(query.lastInsertId().toULongLong());
    }

    //write dict fields
    for (int i = 0; i < records.values.size(); ++i) {
        query.prepare("INSERT INTO NodeDictData(dictID,id,subID,fieldID) "
                      "VALUES(?,?,?,?)");
        query.addBindValue(dictID);
        query.addBindValue(records.values.at(i).at(records.names.indexOf("id")));
        query.addBindValue(records.values.at(i).at(records.names.indexOf("subID")));
        query.addBindValue(flist.at(i));
        if (!query.exec())
            return false;
    }
    db->commit(query);
    emit dictInfoFound(info);

    qDebug() << "new dict" << info.value("name").toString();
    return true;
}
//=============================================================================
//=============================================================================
