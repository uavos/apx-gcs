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
#include "TelemetryImport.h"

#include <App/AppBase.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Database/MissionsDB.h>
#include <Database/TelemetryReqRead.h>
#include <Database/TelemetryReqWrite.h>
#include <Database/VehiclesReqVehicle.h>

TelemetryImport::TelemetryImport()
    : QueueWorker()
{}

void TelemetryImport::exec(Fact *f)
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

void TelemetryImport::run()
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

quint64 TelemetryImport::read(QString fileName)
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
        apxMsgW() << tr("Data exists").append("...");
        return key;
    }
    //check format
    QXmlStreamReader xml(&file);
    while (xml.readNextStartElement()) {
        if (xml.name().compare("telemetry") == 0)
            return read(xml);
        break;
    }
    apxMsgW() << tr("The file format is not correct.");
    return 0;
}
quint64 TelemetryImport::read(QXmlStreamReader &xml)
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
                fields = xml.readElementText().split(',', Qt::SkipEmptyParts);
            else if (tag == "packages")
                readPackages(xml);
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
                apxMsgW() << tr("Data exists").append("...");
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

        //construct fields sequence map for stream decoder, used for 'D' tag
        QList<mandala::uid_t> xml_uid_map;
        for (int i = 0; i < fields.size(); ++i)
            xml_uid_map.append(0);
        for (int i = 0; i < fields.size(); ++i) {
            auto uid = db->mandala_uid(fields.at(i));
            if (!uid) {
                qWarning() << "ignored field" << fields.at(i) << i;
                continue;
            }
            xml_uid_map[i] = uid;
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
                dbSaveEvent(t, name, value, uid, uplink);
                continue;
            }
            if (tag == "U") {
                QString name = xml.attributes().value("name").toString();
                auto uid = db->mandala_uid(name);
                if (uid) {
                    PBase::Values values;
                    values.push_back({uid, (float) xml.readElementText().toDouble()});
                    dbSaveData(t, values, true);
                } else {
                    qWarning() << "ignored field" << name;
                }
                continue;
            }
            if (tag == "D") { // TODO check import/export
                QStringList st = xml.readElementText().split(',', Qt::KeepEmptyParts);
                PBase::Values values;
                uint i = 0;
                for (auto const &s : st) {
                    if (s.isEmpty()) {
                        i++;
                        continue;
                    }
                    if (s.startsWith('#')) {
                        i += s.mid(1).toUInt();
                        continue;
                    }
                    auto uid = xml_uid_map.at(i++);
                    if (uid) {
                        values.push_back({uid, s.toDouble()});
                    }
                }
                if (!values.empty())
                    dbSaveData(t, values, false);
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

QVariantMap TelemetryImport::readSection(QXmlStreamReader &xml)
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

QByteArray TelemetryImport::readXmlPart(QXmlStreamReader &xml)
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

void TelemetryImport::readPackages(QXmlStreamReader &xml)
{
    while (xml.readNextStartElement()) {
        auto tag = xml.name().toString();
        auto data = xml.readElementText().toUtf8();
        if (data.isEmpty()) {
            qWarning() << "data empty";
            continue;
        }
        data = QByteArray::fromBase64(data);
        if (data.isEmpty()) {
            qWarning() << "data base64 decode error";
            continue;
        }
        data = qUncompress(data);
        if (data.isEmpty()) {
            qWarning() << "data uncompress error";
            continue;
        }
        auto var = Fact::parseJsonDocument(data);
        if (var.isNull()) {
            qWarning() << "data parse error";
            continue;
        }
        if (tag == "vehicle") {
            DBReqImportVehicleConfig req(var.value<QVariantMap>());
            req.execSynchronous();
        } else if (tag == "mission") {
            DBReqMissionsSave req(var.value<QVariantMap>());
            req.execSynchronous();
        }
    }
}

quint64 TelemetryImport::dbReadSharedHashId(QString hash)
{
    DBReqTelemetryReadSharedHashId req(hash);
    req.execSynchronous();
    return req.telemetryID;
}
quint64 TelemetryImport::dbSaveID(
    QString vehicleUID, QString callsign, QString comment, bool rec, quint64 timestamp)
{
    DBReqTelemetryNewRecord req(vehicleUID, callsign, comment, rec, timestamp, "");
    req.execSynchronous();
    return req.telemetryID;
}
void TelemetryImport::dbSaveData(quint64 time_ms, PBase::Values values, bool uplink)
{
    DBReqTelemetryWriteData *req = new DBReqTelemetryWriteData(telemetryID, time_ms, values, uplink);
    req->exec();
}
void TelemetryImport::dbSaveEvent(
    quint64 time_ms, const QString &name, const QString &value, const QString &uid, bool uplink)
{
    DBReqTelemetryWriteEvent *req
        = new DBReqTelemetryWriteEvent(telemetryID, time_ms, name, value, uid, uplink);
    req->exec();
}
bool TelemetryImport::dbCommitRecord()
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
