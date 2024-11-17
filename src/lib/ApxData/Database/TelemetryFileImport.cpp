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
#include "TelemetryFileImport.h"
#include "TelemetryFieldAliases.h"
#include "TelemetryFileReader.h"
#include "TelemetryFileWriter.h"

#include <App/App.h>
#include <App/AppDirs.h>

#include <MandalaMetaTree.h>

TelemetryFileImport::TelemetryFileImport(QObject *parent)
    : QTemporaryFile(parent)
{}

bool TelemetryFileImport::import(QString srcFileName)
{
    auto fi = QFileInfo(srcFileName);
    if (!fi.exists() || fi.size() == 0) {
        apxMsgW() << tr("Missing data source").append(':') << fi.absoluteFilePath();
        return false;
    }

    emit progress(0);
    bool rv = false;

    _srcFileName = srcFileName;

    do {
        // get src hash
        {
            QFile src_file(srcFileName);
            if (!src_file.open(QIODevice::ReadOnly)) {
                qWarning() << "failed to open src file" << fi.fileName();
                break;
            }
            _src_hash = TelemetryFileReader::get_hash(&src_file);
            src_file.close();
        }

        // open destination tmp file
        if (!open()) {
            qWarning() << "Failed to open temporary file";
            break;
        }

        // try import native format
        if (fi.suffix() == telemetry::APXTLM_FTYPE) {
            QFile rfile(fi.absoluteFilePath());
            if (!rfile.open(QIODevice::ReadOnly)) {
                apxMsgW() << tr("Failed to open").append(':') << fi.absoluteFilePath();
                break;
            }
            TelemetryFileReader reader(&rfile);
            reader.parse_payload();
            _info = reader.info();

            // check hash exists in info
            auto hash = _info.value("hash").toString();
            if (hash != _src_hash) {
                qDebug() << "hash mismatch" << hash << _src_hash;
                apxMsgW() << tr("Failed to get file hash");
                break;
            }

            //copy data from source
            rfile.seek(0);
            write(rfile.readAll());

            rv = true;
            break;
        }

        // try to import XML formats
        {
            QFile src_file(srcFileName);
            if (!src_file.open(QIODevice::ReadOnly)) {
                qWarning() << "failed to open file" << fi.fileName();
                break;
            }
            //check format
            QXmlStreamReader xml(&src_file);
            if (xml.readNextStartElement()) {
                if (xml.name().compare("telemetry") == 0) {
                    auto format = xml.attributes().value("format").toString();
                    if (format.isEmpty())
                        format = "old";
                    rv = import_xml_v11(xml, format);
                } else if (xml.name().compare("telemetry.gcu.uavos.com") == 0) {
                    rv = import_xml_v9(xml);
                }

                if (rv)
                    break;
            }
        }

        apxMsgW() << tr("Unsupported format").append(':') << fi.fileName();
        break;
    } while (0);

    // finished
    flush();
    close();

    emit progress(-1);
    return rv;
}

QJsonObject TelemetryFileImport::readObject(QXmlStreamReader &xml)
{
    QJsonObject jo;
    while (xml.readNextStartElement()) {
        QString s = xml.readElementText();
        if (s.isEmpty())
            continue;
        jo[xml.name().toString()] = s;
    }
    return jo;
}

QByteArray TelemetryFileImport::readXmlPart(QXmlStreamReader &xml)
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

bool TelemetryFileImport::import_xml_v11(QXmlStreamReader &xml, QString format)
{
    apxMsg() << tr("Importing v10 format").append(':') << QFileInfo(_srcFileName).fileName()
             << format;

    bool ok = true;

    while (ok) {
        //read header
        _info = {};
        QJsonObject &info = _info;

        QHash<QString, QJsonObject> missions;
        QHash<QString, QJsonObject> configs;
        QStringList fields;

        // some default values
        QJsonObject jso_import;
        jso_import["name"] = QFileInfo(_srcFileName).fileName();
        jso_import["title"] = QFileInfo(_srcFileName).baseName();
        jso_import["hash"] = _src_hash;
        jso_import["format"] = format;

        QString tag;
        while (xml.readNextStartElement()) {
            tag = xml.name().toString();
            if (tag == "data")
                break; // stop on data

            if (tag == "info") {
                auto jso = readObject(xml);
                info["timestamp"] = jso["time"].toString().toLongLong();
                info["conf"] = jso["comment"].toString();

                QJsonObject unit;
                unit["uid"] = jso["vehicleUID"].toString();
                unit["name"] = jso["callsign"].toString();
                info["unit"] = unit;
                continue;
            }

            if (tag == "user") {
                auto jso = readObject(xml);
                QJsonObject host;
                host["hostname"] = jso["hostname"].toString();
                host["username"] = jso["username"].toString();
                host["uid"] = jso["machineUID"].toString();
                info["host"] = host;
                continue;
            }

            if (tag == "version") {
                QJsonObject sw;
                sw["version"] = xml.readElementText();
                info["sw"] = sw;
                continue;
            }

            if (tag == "title") {
                jso_import["title"] = xml.readElementText();
                continue;
            }

            if (tag == "timestamp") {
                jso_import["timestamp"] = xml.readElementText();
                continue;
            }
            if (tag == "exported") {
                jso_import["exported"] = xml.readElementText();
                continue;
            }

            if (tag == "fields") {
                fields = xml.readElementText().split(',', Qt::SkipEmptyParts);
                continue;
            }

            if (tag == "packages") {
                while (xml.readNextStartElement()) {
                    auto tag = xml.name().toString();
                    auto data = xml.readElementText().toUtf8();
                    auto hash = xml.attributes().value("hash").toString();
                    if (hash.isEmpty()) {
                        qWarning() << "pkg hash empty";
                        continue;
                    }
                    if (data.isEmpty()) {
                        qWarning() << "pkg empty";
                        continue;
                    }
                    data = QByteArray::fromBase64(data);
                    if (data.isEmpty()) {
                        qWarning() << "pkg base64 decode error";
                        continue;
                    }
                    data = qUncompress(data);
                    if (data.isEmpty()) {
                        qWarning() << "pkg uncompress error";
                        continue;
                    }
                    QJsonParseError err;
                    auto jso = QJsonDocument::fromJson(data, &err).object();
                    if (err.error != QJsonParseError::NoError || jso.isEmpty()) {
                        apxMsgW() << err.errorString();
                        continue;
                    }
                    if (tag == "vehicle") {
                        // unit nodes config
                        configs.insert(hash, jso);
                    } else if (tag == "mission") {
                        // mission data
                        missions.insert(hash, jso);
                    }
                }
                continue;
            }

            // tag not recognized
            xml.skipCurrentElement();
        } // read xml header

        // header parsed
        if (tag != "data" || fields.isEmpty() || info.isEmpty()) {
            apxMsgW() << tr("XML data error");
            qDebug() << info;
            qDebug() << fields;
            ok = false;
            break;
        }

        // save import info
        info["import"] = jso_import;

        // parse title
        {
            auto s = jso_import["title"].toString();
            auto s_ts = s.left(s.indexOf('-'));
            auto s_name = s.mid(s.indexOf('-') + 1);
            auto ts_name = QDateTime::fromString(s_ts, "yyyy_MM_dd_HH_mm_ss_zzz");
            auto s_timestamp = jso_import["timestamp"].toString();

            // parse timestamps, find UTC offset
            auto ts2 = QDateTime::fromString(s_timestamp, Qt::RFC2822Date);
            if (!ts2.isValid()) {
                auto st = s_timestamp.split(' ');
                while (!st.isEmpty()) {
                    if (st.at(0).startsWith("20")) {
                        ts2 = QDateTime::fromString(st.join(' '), "yyyy HH:mm:ss tt");
                        break;
                    }
                    st.removeAt(0);
                }
            }
            if (ts2.isValid()) {
                auto utc_offset = ts2.offsetFromUtc();
                info["utc_offset"] = utc_offset;
                ts_name.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(utc_offset));
                // compare two timestamps
                auto tms_name = ts_name.toMSecsSinceEpoch();
                auto tms = info["timestamp"].toInteger();
                if (tms_name != tms) {
                    apxMsgW() << tr("Timestamp mismatch").append(':') << tms_name << tms
                              << utc_offset;
                    if (tms == 0) {
                        info["timestamp"] = tms_name;
                    }
                }
            }
        } // parse title

        auto timestamp = info["timestamp"].toInteger();
        if (timestamp == 0 || QDateTime::fromMSecsSinceEpoch(timestamp).date().year() < 2010) {
            apxMsgW() << tr("Unknown timestamp");
            qWarning() << info;
            ok = false;
            break;
        }

        // qDebug() << info;
        // qDebug() << fields;

        // init stream writer
        TelemetryFileWriter::Fields fields_stream;
        for (auto f : fields) {
            auto it = mandala::ALIAS_MAP.find(f.toStdString());
            if (it != mandala::ALIAS_MAP.end()) {
                auto uid = it->second;
                qDebug() << f << uid;
                for (auto &m : mandala::meta) {
                    if (m.uid == uid) {
                        auto path = QString(m.path).split('.');
                        path.remove(1, 1);
                        auto name = path.join('.');
                        qDebug() << f << name << m.title << m.units;
                        telemetry::dspec_e dspec = telemetry::dspec_e::f32;
                        fields_stream.push_back({name, m.title, m.units, dspec});
                        f.clear();
                        break;
                    }
                }
                if (f.isEmpty())
                    continue;
            }

            fields_stream.push_back({f, {}, {}});
        }
        TelemetryFileWriter stream(fields_stream);
        stream.init(this, jso_import["name"].toString() + "-import", timestamp, info);

        // init values array to monitor changes
        // std::vector<QVariant> values_s;
        // values_s.resize(fields.size());

        //read <data>
        int progress_s = 0;
        size_t record_count = 0;
        while (xml.readNextStartElement()) {
            //progress
            int progress_v = xml.device()->pos() * 100 / xml.device()->size();
            if (progress_s != progress_v) {
                progress_s = progress_v;
                emit progress(progress_v);
            }

            if (!ok)
                break;

            //read tag
            auto tag = xml.name().toString();
            auto t_tag = xml.attributes().value("t").toUInt();
            if (tag == "D") {
                TelemetryFileWriter::Values values;
                size_t i = 0;
                for (auto const &s : xml.readElementText().split(',', Qt::KeepEmptyParts)) {
                    if (s.isEmpty()) {
                        i++;
                        continue;
                    }
                    if (s.startsWith('#')) {
                        i += s.mid(1).toUInt();
                        continue;
                    }
                    auto field_index = i++;
                    if (field_index >= fields.size())
                        continue;

                    auto v = s.toDouble();
                    if (std::isnan(v) || std::isinf(v))
                        continue;

                    // auto vprev = values_s[field_index];
                    // auto vprev_d = vprev.toDouble();
                    // if (vprev.isValid()) {
                    //     if (vprev_d == v)
                    //         continue;
                    // } else if (v == 0) {
                    //     continue;
                    // }
                    // values_s[field_index] = v;

                    values.push_back({field_index, v});
                }

                if (values.empty())
                    continue;

                stream.write_values(t_tag, values, false);
                record_count++;
                continue;
            }
            //qDebug()<<tag<<t;
            /*if (tag == "E") {
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
            }*/
            // qWarning() << "unknown tag" << tag;
            xml.skipCurrentElement();
        } //read next tag

        qDebug() << "record count" << record_count;
        break;
    } //while ok

    emit progress(0);
    return ok;
}

bool TelemetryFileImport::import_xml_v9(QXmlStreamReader &xml)
{
    apxMsg() << tr("Importing v9 format").append(':') << QFileInfo(_srcFileName).fileName();
    return false;
}
