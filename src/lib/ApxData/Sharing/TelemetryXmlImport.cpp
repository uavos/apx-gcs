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
#include "TelemetryXmlImport.h"
#include "MissionsXml.h"
#include "NodesXml.h"
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>
#include <Fact/Fact.h>
//=============================================================================
TelemetryXmlImport::TelemetryXmlImport()
    : QueueWorker()
{}
//=============================================================================
void TelemetryXmlImport::exec(Fact *f)
{
    QueueWorker::exec(f);
    _fileName = f->descr();
    recordInfo.clear();
    userInfo.clear();
    nodesVehicleInfo.clear();
    sharedHash.clear();
    notes.clear();
    telemetryID = 0;
    start();
}
//=============================================================================
void TelemetryXmlImport::run()
{
    QueueWorker::run();
    title = QFileInfo(_fileName).baseName();
    apxMsg() << tr("Importing").append(":")
             << QString("%1...").arg(QFileInfo(_fileName).fileName());
    quint64 key = read(_fileName);
    emit progress(fact, -1);
    if (!key) {
        if (isInterruptionRequested())
            apxMsg() << tr("Import cancelled");
        else
            apxMsgW() << tr("Import error").append(":") << title;
    }
    result["key"] = key;
    result["title"] = title;
    //qDebug()<<"done";
}
//=============================================================================
//=============================================================================
quint64 TelemetryXmlImport::read(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot read file")
                  << QString("%1:\n%2.").arg(fileName).arg(file.errorString());
        return 0;
    }
    //check if already imported (sharedHash)
    sharedHash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1).toHex().toUpper();
    file.seek(0);
    quint64 key = dbReadSharedHashId(sharedHash);
    if (key) {
        apxMsg() << tr("Data exists").append("...");
        return key;
    }
    //check format
    QXmlStreamReader xml(&file);
    while (xml.readNextStartElement()) {
        if (xml.name() == "telemetry.gcu.uavos.com")
            return readOldFormat(xml);
        if (xml.name() == "telemetry")
            return read(xml);
        break;
    }
    apxMsgW() << tr("The file format is not correct.");
    return 0;
}
quint64 TelemetryXmlImport::read(QXmlStreamReader &xml)
{
    telemetryID = 0;
    int progress_s = 0;
    bool ok = true;

    while (ok) {
        //read header
        QVariantMap stats;
        QStringList fields;
        QString sharedHashExplicit;

        QString tag;
        while (xml.readNextStartElement()) {
            if (isInterruptionRequested())
                break;
            tag = xml.name().toString();
            if (tag == "user")
                userInfo = readSection(xml);
            else if (tag == "info")
                recordInfo = readSection(xml);
            else if (tag == "stats")
                stats = readSection(xml);
            else if (tag == "sharedHash")
                sharedHashExplicit = xml.readElementText();
            else if (tag == "fields")
                fields = xml.readElementText().split(',', QString::SkipEmptyParts);
            else if (tag == "data")
                break;
            else
                xml.skipCurrentElement();
        }
        if (tag != "data" || fields.isEmpty() || recordInfo.isEmpty()) {
            apxMsgW() << tr("No data");
            ok = false;
            break;
        }
        //check existing data by explicit sharedHash
        if (!sharedHashExplicit.isEmpty()) {
            quint64 key = dbReadSharedHashId(sharedHashExplicit);
            if (key) {
                apxMsg() << tr("Data exists").append("...");
                return key;
            }
        }

        title = recordInfo.value("title").toString();
        notes = recordInfo.value("notes").toString();
        //qDebug()<<tag<<userInfo<<info<<stats<<fields;
        quint64 telemetryTime = recordInfo.value("time").toULongLong();

        //register telemetry data file
        telemetryID = dbSaveID(recordInfo.value("vehicleUID").toString(),
                               recordInfo.value("callsign").toString(),
                               recordInfo.value("comment").toString(),
                               false,
                               telemetryTime);
        ok = telemetryID;
        if (!ok)
            break;

        TelemetryDB *db = Database::instance()->telemetry;
        TelemetryDB::TelemetryFieldsMap fieldsMap = db->fieldsMap();

        //construct fieldID map
        QList<quint64> recFieldsMap;
        for (int i = 0; i < fields.size(); ++i)
            recFieldsMap.append(0);
        for (int i = 0; i < fields.size(); ++i) {
            const QString &s = fields.at(i);
            if (!fieldsMap.values().contains(s)) {
                qWarning() << "ignored field" << s;
                continue;
            }
            recFieldsMap[i] = fieldsMap.key(s);
        }

        //read <data>
        while (xml.readNextStartElement()) {
            //progress
            int v_p = xml.device()->pos() * 100 / xml.device()->size();
            if (progress_s != v_p) {
                progress_s = v_p;
                emit progress(fact, v_p);
            }
            if (!ok)
                break;
            if (isInterruptionRequested())
                break;
            //flush queue
            while (db->queueSize() > 1000)
                usleep(1000);
            if (isInterruptionRequested())
                break;
            //read tag
            QString tag = xml.name().toString();
            quint64 t = xml.attributes().value("t").toULongLong();
            //qDebug()<<tag<<t;
            if (tag == "E") {
                QString name = xml.attributes().value("name").toString();
                QString uid = xml.attributes().value("uid").toString();
                bool uplink = xml.attributes().value("uplink").toUInt();
                QString value = xml.readElementText();
                if (name == "nodes" || name == "mission") {
                    if (!xml.readNextStartElement())
                        continue;
                    tag = xml.name().toString();
                    //qDebug()<<tag;
                    if (tag == name) {
                        if (name == "nodes") {
                            dbSaveNodes(t, telemetryTime, title, readXmlPart(xml));
                        } else if (name == "mission") {
                            dbSaveMission(t, telemetryTime, title, readXmlPart(xml), uplink);
                        }
                        continue;
                    }
                    qWarning() << "missing event data for" << name;
                    //continue tags processing
                    t = xml.attributes().value("t").toULongLong();
                } else {
                    //event with no xml data injected
                    dbSaveEvent(t, name, value, uid, uplink);
                    continue;
                }
            }
            if (tag == "U") {
                QString name = xml.attributes().value("name").toString();
                quint64 fieldID = fieldsMap.key(name);
                if (fieldID) {
                    dbSaveData(t, fieldID, xml.readElementText().toDouble(), true);
                } else {
                    qWarning() << "ignored field" << name;
                }
                continue;
            }
            if (tag == "D") {
                QStringList st = xml.readElementText().split(',', QString::KeepEmptyParts);
                for (int i = 0; i < st.size(); ++i) {
                    const QString &s = st.at(i);
                    if (s.isEmpty())
                        continue;
                    quint64 fieldID = recFieldsMap.at(i);
                    if (fieldID) {
                        dbSaveData(t, fieldID, s.toDouble(), false);
                    }
                }

                continue;
            }
            qWarning() << "unknown tag" << tag;
            xml.skipCurrentElement();
        } //read next tag
        break;
    } //while ok

    if (isInterruptionRequested()) {
        ok = false;
    } else if (ok && telemetryID) {
        emit progress(fact, 0);
        ok = dbCommitRecord();
    }
    return ok ? telemetryID : 0;
}
//=============================================================================
QVariantMap TelemetryXmlImport::readSection(QXmlStreamReader &xml)
{
    QVariantMap info;
    while (xml.readNextStartElement()) {
        QString s = xml.readElementText();
        if (s.isEmpty())
            continue;
        info[xml.name().toString()] = s;
    }
    return info;
}
//=============================================================================
//=============================================================================
//=============================================================================
//=============================================================================
quint64 TelemetryXmlImport::readOldFormat(QXmlStreamReader &xml)
{
    qWarning() << tr("Importing old format") << QString("(%1)").arg(QFileInfo(_fileName).suffix());
    //tag: telemetry.gcu.uavos.com
    telemetryID = 0;
    int progress_s = 0;
    bool ok = true;
    while (ok) {
        //get timestamp from file
        quint64 timestamp = 0;
        //try UTC attr
        if (xml.attributes().hasAttribute("UTC")) {
            QDateTime ftime = QDateTime::fromString(xml.attributes().value("UTC").toString(),
                                                    Qt::ISODate);
            if (ftime.isValid())
                timestamp = ftime.toMSecsSinceEpoch();
            //qDebug()<<xml.attributes().value("UTC").toString();
            //qDebug()<<ftime.toUTC().toString(Qt::ISODate);
            //qDebug()<<timestamp;
        }
        ok = timestamp != 0;
        if (!ok) {
            apxMsgW() << tr("Can't get timestamp for file") << QString("'%1'").arg(title);
            break;
        }
        //find title and comment from file name
        QString comment = title;
        QString sts = QDateTime::fromMSecsSinceEpoch(timestamp).toString("yyyy_MM_dd_HH_mm");
        if (comment.startsWith(sts))
            comment.remove(0, sts.size());
        if (comment.startsWith('_'))
            comment.remove(0, 1);

        QStringList st = title.split('_');
        if (st.size() >= 7) {
            QStringList st2;
            for (int i = 6; i < st.size(); ++i)
                st2.append(st.at(i));
            title = st2.join('_');
            QString snum = st.at(5);
            if (comment.startsWith(snum))
                comment.remove(0, snum.size());
            if (comment.startsWith('_'))
                comment.remove(0, 1);
        }
        if (title.isEmpty()) {
            title = "UNKNOWN";
            apxMsgW() << tr("Callsign").append(":") << title;
        }
        if (comment == title)
            comment.clear();

        //register telemetry data file
        telemetryID = dbSaveID(QString(), title == "LOCAL" ? "" : title, comment, false, timestamp);
        ok = telemetryID;
        if (!ok)
            break;

        //collect available DB fields
        TelemetryDB *db = Database::instance()->telemetry;
        TelemetryDB::TelemetryFieldsMap fieldsMap = db->fieldsMap();

        if (isInterruptionRequested())
            break;

        //read XML file data
        QList<quint64> recFieldsMap;
        QList<double> recValues;
        int dl_timestamp_idx = -1;
        uint time_ms = 0, time_start = 0;
        uint time_s = 0;
        while (xml.readNextStartElement()) {
            if (isInterruptionRequested())
                break;
            const QString tag = xml.name().toString();
            if (tag == "mandala") {
                //-----------------------------------------------------------
                while (xml.readNextStartElement()) {
                    if (xml.name() == "fields") {
                        const QStringList &st = xml.readElementText().split(',');
                        recFieldsMap.clear();
                        for (int i = 0; i < st.size(); ++i)
                            recFieldsMap.append(0);
                        for (int i = 0; i < st.size(); ++i) {
                            const QString &s = st.at(i);
                            if (!fieldsMap.values().contains(s)) {
                                qWarning() << "ignored field" << s;
                                continue;
                            }
                            recFieldsMap[i] = fieldsMap.key(s);
                            if (s == "dl_timestamp")
                                dl_timestamp_idx = i;
                        }
                    } else
                        xml.skipCurrentElement();
                }
            } else if (tag == "D" || tag == "U" || tag == "S") {
                //-----------------------------------------------------------
                if (xml.attributes().isEmpty()) {
                    //values downstream
                    const QStringList &st = xml.readElementText().split(',');
                    if (dl_timestamp_idx >= 0 && dl_timestamp_idx < st.size()
                        && st.at(dl_timestamp_idx).size()) {
                        //find dl_timestamp
                        uint t_ms = st.at(dl_timestamp_idx).toUInt();
                        uint dt_ms = t_ms - time_s;
                        time_s = t_ms;
                        if (dt_ms < 1)
                            dt_ms = 100;
                        else if (dt_ms > 1000)
                            dt_ms = 1000;
                        time_ms += dt_ms;
                        if (!time_start) {
                            time_start = time_ms;
                            time_ms -= time_start;
                        }
                        //append data
                        //db->transaction(query);
                        for (int i = 0; i < recFieldsMap.size(); ++i) {
                            bool brec = false;
                            double v = st.at(i).toDouble(&brec);
                            if (!brec)
                                v = 0;
                            if (i >= recValues.size()) {
                                recValues.append(v);
                                brec = true;
                            } else if (brec && recValues.at(i) == v)
                                brec = false;
                            if (!brec)
                                continue;
                            recValues[i] = v;
                            //write value update
                            quint64 fieldID = recFieldsMap.at(i);
                            if (fieldID) {
                                dbSaveData(time_ms, fieldID, v, false);
                            }
                            //ok=db->writeField(time_ms,recFieldsMap.at(i),v,false);
                            //if(!ok)break;
                        }
                        //db->commit(query);
                    }
                } else if (xml.attributes().hasAttribute("f")) {
                    //var update downlink/uplink
                    const QString &fname = xml.attributes().value("f").toString();
                    quint64 fieldID = fieldsMap.key(fname);
                    if (fname == "xpdr") {
                        dbSaveEvent(time_ms, "xpdr", xml.readElementText(), "", false);
                        //db->writeEvent(time_ms,"xpdr","raw",false,QByteArray::fromHex(xml.readElementText().toUtf8()));
                    } else if (fieldID == 0 || fname.startsWith("gcu_"))
                        xml.skipCurrentElement();
                    else {
                        double v = xml.readElementText().toDouble();
                        int i = recFieldsMap.indexOf(fieldID);
                        bool bUplink = tag == "U";
                        if (bUplink == false && i >= 0 && i < recValues.size())
                            recValues[i] = v;
                        //write value update
                        if (fieldID) {
                            dbSaveData(time_ms, fieldID, v, bUplink);
                        }
                        //ok=db->writeField(time_ms,fieldID,v,bUplink);
                        //if(!ok)break;
                    }
                }
            } else if (tag == "msg") {
                //-----------------------------------------------------------
                const QString &sn = xml.attributes().value("sn").toString();
                QString s = "[" + xml.attributes().value("node_name").toString() + "]";
                s += xml.readElementText();
                dbSaveEvent(time_ms, "msg", s, sn, false);
                //db->writeEvent(time_ms,"msg",s,false,sn);
            } else if (tag == "notes") {
                //-----------------------------------------------------------
                notes = xml.readElementText().simplified();
            } else if (tag == "node_conf") {
                //-----------------------------------------------------------
                const QString &sn = xml.attributes().value("sn").toString();
                const QString &stitle = xml.attributes().value("name").toString();
                QString fname = xml.attributes().value("f").toString();
                bool bArray = false;
                if (fname.contains('[')) {
                    fname = fname.left(fname.indexOf('['));
                    bArray = true;
                }
                while (xml.readNextStartElement()) {
                    if (xml.name() == "value") {
                        QString fxname = fname;
                        if (xml.attributes().hasAttribute("name")) {
                            fxname.append(
                                QString("_%1").arg(xml.attributes().value("name").toString()));
                        } else if (xml.attributes().hasAttribute("idx")) {
                            fxname.append(
                                QString("_%1").arg(xml.attributes().value("idx").toString()));
                        }
                        QString v = xml.readElementText();
                        if (fxname.endsWith("code") && (!v.isEmpty())) {
                            v = qCompress(v.toUtf8(), 9).toHex().toUpper();
                        }
                        const QString &s = QString("%1/%2=%3").arg(stitle).arg(fxname).arg(v);
                        dbSaveEvent(time_ms, "conf", s, sn, true);

                    } else
                        xml.skipCurrentElement();
                }
            } else if (tag == "nodes") {
                //-----------------------------------------------------------
                dbSaveNodes(time_ms, timestamp, title, readXmlPart(xml));
            } else if (tag == "mission") {
                //-----------------------------------------------------------
                dbSaveMission(time_ms, timestamp, title, readXmlPart(xml), true);
            } else {
                //-----------------------------------------------------------
                //unknown tag
                xml.skipCurrentElement();
            }
            //-----------------------------------------------------------
            int v_p = xml.device()->pos() * 100 / xml.device()->size();
            if (progress_s != v_p) {
                progress_s = v_p;
                emit progress(fact, v_p);
            }
            if (!ok)
                break;

            if (isInterruptionRequested())
                break;

            //flush queue
            while (db->queueSize() > 1000)
                usleep(1000);
        }

        if ((time_ms - time_start) == 0) {
            apxMsgW() << tr("No data in file");
            ok = false;
            break;
        }

        break; //while ok
    }
    //qDebug()<<ok<<isInterruptionRequested();
    if (isInterruptionRequested()) {
        ok = false;
    } else if (ok && telemetryID) {
        emit progress(fact, 0);
        ok = dbCommitRecord();
    }
    return ok ? telemetryID : 0;
}
//=============================================================================
QByteArray TelemetryXmlImport::readXmlPart(QXmlStreamReader &xml)
{
    const QString tag = xml.name().toString();
    QByteArray xmlPart;
    QXmlStreamWriter writer(&xmlPart);
    writer.writeCurrentToken(xml); //start tag
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement() && xml.name() == tag)
            break;
        writer.writeCurrentToken(xml);
    }
    writer.writeCurrentToken(xml); //end tag
    return xmlPart;
}
//=============================================================================
//=============================================================================
quint64 TelemetryXmlImport::dbReadSharedHashId(QString hash)
{
    DBReqTelemetryReadSharedHashId req(hash);
    req.execSynchronous();
    return req.telemetryID;
}
quint64 TelemetryXmlImport::dbSaveID(
    QString vehicleUID, QString callsign, QString comment, bool rec, quint64 timestamp)
{
    DBReqTelemetryNewRecord req(vehicleUID, callsign, comment, rec, timestamp);
    req.execSynchronous();
    return req.telemetryID;
}
void TelemetryXmlImport::dbSaveData(quint64 time_ms, quint64 fieldID, double value, bool uplink)
{
    DBReqTelemetryWriteData *req = new DBReqTelemetryWriteData(telemetryID,
                                                               time_ms,
                                                               fieldID,
                                                               value,
                                                               uplink);
    req->exec();
}
void TelemetryXmlImport::dbSaveEvent(
    quint64 time_ms, const QString &name, const QString &value, const QString &uid, bool uplink)
{
    DBReqTelemetryWriteEvent *req
        = new DBReqTelemetryWriteEvent(telemetryID, time_ms, name, value, uid, uplink);
    req->exec();
}
void TelemetryXmlImport::dbSaveMission(
    quint64 time_ms, quint64 timestamp, const QString &title, const QByteArray &data, bool uplink)
{
    if (data.isEmpty())
        return;
    MissionsXmlImport req(title, "");
    req.defaultTime = timestamp + time_ms;
    req.data = data;
    req.execSynchronous();
    QString hash = req.info.value("hash").toString();
    if (hash.isEmpty())
        return;
    dbSaveEvent(time_ms, "mission", req.info.value("title").toString(), hash, uplink);
}
void TelemetryXmlImport::dbSaveNodes(quint64 time_ms,
                                     quint64 timestamp,
                                     const QString &title,
                                     const QByteArray &data)
{
    if (data.isEmpty())
        return;
    NodesXmlImport req(title, "");
    req.defaultTime = timestamp + time_ms;
    req.data = data;
    req.execSynchronous();
    QString hash = req.info.value("hash").toString();
    if (hash.isEmpty())
        return;
    nodesVehicleInfo = req.vehicleInfo;
    //check vehicle info
    dbSaveEvent(time_ms, "nodes", req.info.value("title").toString(), hash, false);
}
bool TelemetryXmlImport::dbCommitRecord()
{
    bool ok = true;
    while (telemetryID) {
        //make cache
        QString hash;
        {
            DBReqTelemetryMakeStats req(telemetryID);
            ok = req.execSynchronous();
            if (!ok)
                break;
            hash = req.stats.value("hash").toString();
        }
        bool exists = false;
        //check if same data bundle exists
        {
            DBReqTelemetryFindDataHash req(hash);
            ok = req.execSynchronous();
            if (!ok)
                break;
            if (req.telemetryID) {
                apxMsgW() << tr("Same data exists");
                {
                    //delete imported data
                    DBReqTelemetryDeleteRecord reqDel(telemetryID);
                    ok = reqDel.execSynchronous();
                }
                telemetryID = req.telemetryID;
                exists = true;
            }
        }

        //shared data
        if (!(sharedHash.isEmpty() && userInfo.isEmpty())) {
            QVariantMap info;
            if (!exists)
                info = userInfo;
            info["sharedHash"] = sharedHash;
            //qDebug()<<sharedHash;
            DBReqTelemetryWriteSharedInfo req(telemetryID, info);
            ok = req.execSynchronous();
            if (!ok)
                break;
        }

        if (exists)
            break;
        //remove from trash and update info
        {
            QVariantMap info;
            if (recordInfo.isEmpty()) {
                info["callsign"] = nodesVehicleInfo.value("callsign");
                info["vehicleUID"] = nodesVehicleInfo.value("uid");
                info = DatabaseRequest::filterNullValues(info);
            } else {
                info = recordInfo;
                info.remove("time");
            }
            if (!info.value("callsign").toString().isEmpty()) {
                QString callsign = info.value("callsign").toString();
                title = title.remove(callsign, Qt::CaseInsensitive).simplified();
                notes = notes.append(' ').append(title).simplified();
                title = callsign;
            }

            notes = notes.remove(title, Qt::CaseInsensitive).replace('_', ' ').simplified();
            if (!notes.isEmpty()) {
                info["notes"] = notes;
            }
            DBReqTelemetryWriteInfo req(telemetryID, info, true);
            req.execSynchronous();
        }
        break;
    }
    if (isInterruptionRequested()) {
        ok = false;
    } else if (!ok) {
        qWarning() << "error";
    }
    return ok;
}
//=============================================================================
