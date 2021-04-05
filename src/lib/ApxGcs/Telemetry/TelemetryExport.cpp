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
#include "TelemetryExport.h"

#include <App/AppBase.h>
#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Database/Database.h>
#include <Database/MissionsDB.h>
#include <Database/TelemetryReqRead.h>
#include <Database/VehiclesReqVehicle.h>

TelemetryExport::TelemetryExport()
    : QueueWorker()
{}

void TelemetryExport::exec(Fact *f)
{
    QueueWorker::exec(f);
    _fileName = f->descr();
    _telemetryID = f->value().toULongLong();
    start();
}

void TelemetryExport::run()
{
    QueueWorker::run();
    QString title = QFileInfo(_fileName).fileName();
    apxMsg() << tr("Exporting").append(":") << QString("%1...").arg(title);
    bool ok = write(_telemetryID, _fileName);
    emit progress(fact, -1);
    if (ok)
        return;
    if (isInterruptionRequested())
        apxMsg() << tr("Export cancelled");
    else
        apxMsgW() << tr("Export error").append(":") << title;
}

bool TelemetryExport::write(quint64 telemetryID, QString fileName)
{
    {
        DBReqTelemetryMakeStats req(telemetryID);
        if (!req.execSynchronous())
            return false;
    }
    DBReqTelemetryReadData req(telemetryID);
    connect(this, &QueueWorker::stopRequested, &req, &DatabaseRequest::discard, Qt::QueuedConnection);
    if (!req.execSynchronous())
        return false;

    QVariantMap info = req.info;

    if (req.records.values.size() <= 0) {
        apxMsgW() << tr("Nothing to export");
    }

    QString sharedHash;
    {
        DBReqTelemetryReadSharedHashId req(telemetryID);
        if (!req.execSynchronous())
            return false;
        sharedHash = req.hash;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        apxMsgW() << tr("Cannot write file").append(":") << fileName
                  << QString("(%1)").arg(file.errorString());
        return false;
    }
    bool ok = false;
    QString ftype = QFileInfo(fileName).suffix();
    if (ftype == "csv") {
        ok = writeCSV(&file, req);
    } else {
        ok = writeXml(&file, req, QFileInfo(fileName).completeBaseName(), sharedHash, info);
    }
    file.close();
    if (ok) {
        apxMsg() << tr("Exported telemetry").append(':') << QFileInfo(fileName).fileName();
    } else {
        file.remove();
    }
    return ok;
}

bool TelemetryExport::writeXml(QFile *file_p,
                               const DBReqTelemetryReadData &req,
                               const QString &title,
                               const QString &sharedHash,
                               const QVariantMap &info)
{
    QXmlStreamWriter stream(file_p);
    stream.setAutoFormatting(true);
    stream.setAutoFormattingIndent(2);
    stream.writeStartDocument();
    stream.writeStartElement("telemetry");
    stream.writeAttribute("href", "http://www.uavos.com/");
    stream.writeAttribute("format", "11");

    stream.writeTextElement("title", title);
    stream.writeTextElement("timestamp",
                            QDateTime::fromMSecsSinceEpoch(info.value("time").toLongLong())
                                .toString(Qt::RFC2822Date));
    stream.writeTextElement("exported", QDateTime::currentDateTime().toString(Qt::RFC2822Date));
    stream.writeTextElement("version", AppBase::version());
    if (!sharedHash.isEmpty())
        stream.writeTextElement("sharedHash", sharedHash);

    stream.writeStartElement("user");
    stream.writeTextElement("machineUID", AppBase::machineUID());
    stream.writeTextElement("hostname", AppBase::hostname());
    stream.writeTextElement("username", AppBase::username());
    stream.writeEndElement();

    //telemetry record info
    stream.writeStartElement("info");
    for (auto key : info.keys()) {
        if (key == "trash")
            continue;
        stream.writeTextElement(key, info.value(key).toString());
    }
    stream.writeEndElement();

    //fields list
    QList<quint64> fidList;
    QStringList fieldNames;
    for (auto key : req.fieldNames.keys()) {
        fidList.append(key);
        fieldNames.append(req.fieldNames.value(key));
    }
    stream.writeTextElement("fields", fieldNames.join(','));

    const int iTime = req.records.names.indexOf("time");
    const int iType = req.records.names.indexOf("type");
    const int iName = req.records.names.indexOf("name");
    const int iValue = req.records.names.indexOf("value");
    const int iUID = req.records.names.indexOf("uid");

    // vehicle configs and missions
    QSet<QString> configs, missions;
    for (auto const &r : req.records.values) {
        const auto type = r.at(iType).toUInt();
        if (type < 2)
            continue;
        auto name = r.at(iName).toString();
        auto hash = r.at(iUID).toString();
        if (hash.isEmpty())
            continue;
        if (name == "mission") {
            missions.insert(hash);
        } else if (name == "nodes") {
            configs.insert(hash);
        }
    }
    stream.writeStartElement("packages");
    for (auto const &hash : configs) {
        DBReqLoadVehicleConfig req(hash);
        if (!req.execSynchronous()) {
            apxMsgW() << tr("Can't export config") << hash;
            continue;
        }
        QByteArray data = QJsonDocument::fromVariant(req.config()).toJson(QJsonDocument::Compact);

        stream.writeStartElement("vehicle");
        stream.writeAttribute("hash", hash);
        stream.writeCharacters(qCompress(data, 9).toBase64());
        stream.writeEndElement();
    }
    for (auto const &hash : missions) {
        DBReqMissionsLoad req(hash);
        if (!req.execSynchronous()) {
            apxMsgW() << tr("Can't export mission") << hash;
            continue;
        }
        QByteArray data = QJsonDocument::fromVariant(req.mission()).toJson(QJsonDocument::Compact);

        stream.writeStartElement("mission");
        stream.writeAttribute("hash", hash);
        stream.writeCharacters(qCompress(data, 9).toBase64());
        stream.writeEndElement();
    }
    stream.writeEndElement();

    //write data
    stream.setAutoFormattingIndent(0);
    stream.writeStartElement("data");

    bool ok = true;
    QStringList values;
    quint64 valuesTime = 0;
    values.reserve(fidList.size());
    int progress_s = 0;
    const int cnt = req.records.values.size();
    for (int i = 0; ok && i < cnt; ++i) {
        const auto &r = req.records.values.at(i);
        const auto time = r.at(iTime).toULongLong();
        const auto type = r.at(iType).toUInt();
        //progress
        int v_p = i * 100 / cnt;
        if (progress_s != v_p) {
            progress_s = v_p;
            emit progress(fact, v_p);
        }
        //record
        if (type == 0) { //downlink
            if (values.isEmpty())
                valuesTime = time;
            else if (valuesTime != time) {
                writeDownlink(stream, valuesTime, values);
                valuesTime = time;
                values.clear();
            }
            int vi = fidList.indexOf(r.at(iName).toULongLong());
            ok = vi >= 0;
            if (!ok)
                break;
            while (vi >= values.size())
                values.append("");
            if (!values.at(vi).isEmpty()) {
                qWarning() << "duplicate value" << time << fieldNames.at(vi);
            }
            values[vi] = r.at(iValue).toString();
            continue;
        }
        if (!values.isEmpty()) {
            writeDownlink(stream, valuesTime, values);
            values.clear();
        }
        switch (type) {
        case 1: { //uplink
            int vi = fidList.indexOf(r.at(iName).toULongLong());
            ok = vi >= 0;
            if (!ok)
                break;
            writeUplink(stream, time, fieldNames.at(vi), r.at(iValue).toString());
        } break;
        case 2:
        case 3: { //event
            QString name = r.at(iName).toString();
            QString uid = r.at(iUID).toString();
            writeEvent(stream, time, name, r.at(iValue).toString(), uid, type == 3);
        } break;
        }
    }
    if (!values.isEmpty()) {
        writeDownlink(stream, valuesTime, values);
    }
    stream.writeEndElement(); //data
    stream.writeEndElement(); //telemetry

    return ok;
}

void TelemetryExport::writeDownlink(QXmlStreamWriter &stream,
                                    quint64 time,
                                    const QStringList &values)
{
    stream.writeStartElement("D");
    stream.writeAttribute("t", QString::number(time));
    // combine zero values
    uint n = 0;
    QStringList st;
    for (auto const &v : values) {
        if (v.isEmpty()) {
            n++;
            continue;
        }
        if (n > 2) {
            st.append("#" + QString::number(n));
            n = 0;
        } else if (n > 0) {
            while (n--) {
                st.append(QString());
            }
            n = 0;
        }
        st.append(v);
    }
    stream.writeCharacters(st.join(','));
    stream.writeEndElement();
}
void TelemetryExport::writeUplink(QXmlStreamWriter &stream,
                                  quint64 time,
                                  const QString &name,
                                  const QString &value)
{
    stream.writeStartElement("U");
    stream.writeAttribute("t", QString::number(time));
    stream.writeAttribute("name", name);
    stream.writeCharacters(value);
    stream.writeEndElement();
}
void TelemetryExport::writeEvent(QXmlStreamWriter &stream,
                                 quint64 time,
                                 const QString &name,
                                 const QString &value,
                                 const QString &uid,
                                 bool uplink)
{
    stream.writeStartElement("E");
    stream.writeAttribute("t", QString::number(time));
    stream.writeAttribute("name", name);
    if (!uid.isEmpty())
        stream.writeAttribute("uid", uid);
    if (uplink)
        stream.writeAttribute("uplink", "1");
    stream.writeCharacters(value);
    stream.writeEndElement();
}

bool TelemetryExport::writeCSV(QFile *file_p, DBReqTelemetryReadData &req)
{
    QTextStream stream(file_p);

    //fields list
    QList<quint64> fidList;
    QStringList fieldNames;
    fieldNames << "time";
    foreach (quint64 fid, req.fieldNames.keys()) {
        fidList.append(fid);
        fieldNames.append(req.fieldNames.value(fid));
    }
    stream << fieldNames.join(',') << QString("\n");

    //write data
    QStringList values;
    for (int i = 0; i < fieldNames.size(); ++i) {
        values.append("0");
    }

    int iTime = req.records.names.indexOf("time");
    int iType = req.records.names.indexOf("type");
    int iName = req.records.names.indexOf("name");
    int iValue = req.records.names.indexOf("value");

    quint64 t0 = 0;
    quint64 t = 0;
    quint64 t_s = 0;
    QHash<quint64, double> fvalues;
    for (int i = 0; i < req.records.values.size(); ++i) {
        const QVariantList &r = req.records.values.at(i);
        if (r.isEmpty())
            continue;

        //time
        t = r.at(iTime).toULongLong();
        if (i == 0)
            t0 = t;
        t -= t0;

        if (t_s != t) {
            stream << values.join(',') << QString("\n");
            t_s = t;
        }

        quint64 fid = 0;

        switch (r.at(iType).toUInt()) {
        case 0: {
            //downlink data
            fid = r.at(iName).toULongLong();
        } break;
        case 1: {
            //uplink data
            fid = r.at(iName).toULongLong();
        } break;
        }

        if (!fid)
            continue;
        int vidx = fidList.indexOf(fid) + 1;
        if (vidx < 0)
            continue;
        values[0] = QString::number(t);
        values[vidx] = r.at(iValue).toString();
    }
    //final data tail at max time
    if (t_s != t) {
        stream << values.join(',') << QString("\n");
    }

    //finish
    stream.flush();
    return true;
}
