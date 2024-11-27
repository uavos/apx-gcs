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
#include "TelemetryFileEvents.h"
#include "TelemetryFileReader.h"
#include "TelemetryFileWriter.h"

#include <App/App.h>
#include <App/AppDirs.h>

#include <ApxMisc/JsonHelpers.h>
#include <MandalaMetaTree.h>

TelemetryFileImport::TelemetryFileImport(QObject *parent)
    : QTemporaryFile(parent)
{}

bool TelemetryFileImport::import_telemetry(QString srcFileName)
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
                    rv = import_telemetry_v11(xml, format);
                } else if (xml.name().compare("telemetry.gcu.uavos.com") == 0) {
                    rv = import_telemetry_v9(xml);
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
    QJsonObject jso;
    while (xml.readNextStartElement()) {
        jso[xml.name().toString()] = xml.readElementText();
    }
    return json::filter_names(jso);
}

bool TelemetryFileImport::import_telemetry_v11(QXmlStreamReader &xml, QString format)
{
    apxMsg() << tr("Importing v10 format").append(':') << QFileInfo(_srcFileName).fileName()
             << format;

    bool ok = true;

    while (ok) {
        //read header
        _info = {};
        QJsonObject &info = _info;

        std::map<QString, QJsonObject> missions;
        std::map<QString, QJsonObject> configs;
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

            if (tag == "packages") { // 11 only
                while (xml.readNextStartElement()) {
                    auto tag = xml.name().toString();
                    auto hash = xml.attributes().value("hash").toString();
                    auto data = xml.readElementText().toUtf8();
                    if (hash.isEmpty()) {
                        qWarning() << "pkg hash empty" << tag;
                        continue;
                    }
                    if (data.isEmpty()) {
                        qWarning() << "pkg empty" << tag;
                        continue;
                    }
                    data = QByteArray::fromBase64(data);
                    if (data.isEmpty()) {
                        qWarning() << "pkg base64 decode error" << tag;
                        continue;
                    }
                    data = qUncompress(data);
                    if (data.isEmpty()) {
                        qWarning() << "pkg uncompress error" << tag;
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
                        configs[hash] = jso;
                    } else if (tag == "mission") {
                        // mission data
                        missions[hash] = jso;
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
        jso_import = json::filter_names(jso_import);
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
        std::vector<TelemetryFileWriter::Field> fields_stream;
        for (auto f : fields) {
            auto it = mandala::ALIAS_MAP.find(f.toStdString());
            if (it != mandala::ALIAS_MAP.end()) {
                auto uid = it->second;
                for (auto &m : mandala::meta) {
                    if (m.uid != uid)
                        continue;
                    auto path = QString(m.path).split('.');
                    path.remove(1, 1);
                    auto name = path.join('.');
                    auto dspec = mandala::dspec_for_uid(uid);
                    fields_stream.push_back({name, {m.title, m.units}, dspec});
                    // qDebug() << f << name << m.title << m.units << (uint) dspec;
                    f.clear();
                    break;
                }
                if (f.isEmpty())
                    continue;
            }
            // qDebug() << f;
            // push field as-is (old flat format)
            fields_stream.push_back({f, {}, telemetry::dspec_e::f32});
        }
        TelemetryFileWriter stream;
        stream.init(this, jso_import["name"].toString() + "-import", timestamp, info);

        // -------------------------------------------
        // read <data>
        int progress_s = 0;      // to don't update unchanged
        size_t record_count = 0; // just stats at the end
        uint32_t time_tag = 0;   // current timestamp [ms]
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
            const auto tag = xml.name().toString();
            if (xml.attributes().hasAttribute("t"))
                time_tag = xml.attributes().value("t").toUInt();

            // downlink data
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

                    const auto &field = fields_stream[field_index];
                    values[&field] = v;
                }

                if (values.empty())
                    continue;

                stream.write_values(time_tag, values, false);
                record_count++;
                continue;
            }

            // uplink data
            if (tag == "U") {
                const auto name = xml.attributes().value("name").toString();
                // find field index by name
                int field_index = -1;
                for (auto const &f : fields) {
                    field_index++;
                    if (f == name)
                        break;
                }
                if (field_index < 0) {
                    qWarning() << "ignored field" << name;
                    continue;
                }
                TelemetryFileWriter::Values values;
                const auto &field = fields_stream[field_index];
                values[&field] = (float) xml.readElementText().toDouble();
                stream.write_values(time_tag, values, true);
                record_count++;
                continue;
            }

            // event
            if (tag == "E") {
                const auto evt_name = xml.attributes().value("name").toString();
                const auto uid = xml.attributes().value("uid").toString();
                bool uplink = xml.attributes().value("uplink").toUInt();
                const auto value = xml.readElementText();
                if (evt_name.isEmpty() || uid.isEmpty()) {
                    qWarning() << "unknown event" << evt_name << value;
                    continue;
                }
                // qDebug() << "event" << evt_name << value << uid;
                if (evt_name == "msg") {
                    stream.write_evt(time_tag, &telemetry::EVT_MSG, {value, uid});
                    continue;
                }
                if (evt_name == "mission") {
                    auto it = missions.find(uid);
                    if (it == missions.end())
                        continue;

                    auto hash = it->first;
                    auto mission = it->second;
                    auto title = mission["title"].toString();
                    QJsonObject jso;
                    jso["import"] = jso_import;
                    jso["time"] = info["timestamp"]; // from xml file
                    jso["mission"] = it->second;     // encapsulate content
                    // qDebug() << value << jso;
                    stream.write_jso(time_tag, evt_name, jso, uplink);
                    continue;
                }
                if (evt_name == "nodes") {
                    auto it = configs.find(uid);
                    if (it == configs.end())
                        continue;

                    QJsonObject jso = it->second;
                    jso["import"] = jso_import;
                    // qDebug() << value << jso;
                    stream.write_jso(time_tag, evt_name, jso, uplink);
                    continue;
                }

                continue;
            } // <E> event

            // mission
            if (tag == "mission") {
                auto jso = import_mission(xml);
                if (jso.isEmpty())
                    continue;
                jso["import"] = json::merge(jso["import"].toObject(), jso_import);
                qDebug() << tag << jso;
                stream.write_jso(time_tag, tag, jso, false);
                continue;
            }

            // qWarning() << "unknown tag" << tag;
            xml.skipCurrentElement();
        } //read next tag

        qDebug() << "record count" << record_count;
        break;
    } //while ok

    emit progress(0);
    return ok;
}

bool TelemetryFileImport::import_telemetry_v9(QXmlStreamReader &xml)
{
    apxMsg() << tr("Importing v9 format").append(':') << QFileInfo(_srcFileName).fileName();
    return false;
}

QJsonObject TelemetryFileImport::import_mission(QXmlStreamReader &xml)
{
    // already in <mission ...> tag
    if (xml.name().toString() != "mission") {
        qWarning() << "not a mission tag" << xml.name();
        return {};
    }

    auto format = xml.attributes().value("format").toString();

    QJsonObject jso_import;
    jso_import["format"] = format.isEmpty() ? "old" : format;
    jso_import["title"] = xml.attributes().value("title").toString();
    jso_import["version"] = xml.attributes().value("version").toString();

    QJsonObject jso_mission;
    QHash<QString, std::pair<QString, QString>> map_array{
        // tag, {sub_tag, name}
        {"waypoints", {"waypoint", "wp"}},
        {"points", {"point", "pi"}},
        {"runways", {"runway", "rw"}},
        {"taxiways", {"taxiway", "tw"}},
    };
    QHash<QString, QString> map_fields{
        {"latitude", "lat"},
        {"longitude", "lon"},
        {"HMSL", "hmsl"},
        {"turnR", "radius"},
        {"time", "timeout"},
        {"turn", "type"},
        {"appType", "type"},
        {"distApp", "approach"},
    };

    qDebug() << "import mission" << jso_import["title"];

    size_t record_count = 0;
    while (xml.readNextStartElement()) {
        const auto tag = xml.name().toString();
        if (tag == "timestamp") {
            jso_import["timestamp"] = xml.readElementText();
            continue;
        }
        if (tag == "exported") {
            jso_import["exported"] = xml.readElementText();
            continue;
        }
        if (tag == "version") {
            jso_import["version"] = xml.readElementText();
            continue;
        }
        if (tag == "info") {
            auto jso = readObject(xml);
            jso_mission["title"] = jso["title"];
            jso_mission["time"] = jso["time"];
            continue;
        }

        // items
        if (map_array.contains(tag)) {
            auto [sub_tag, name] = map_array[tag];
            QJsonArray jsa;
            while (xml.readNextStartElement()) {
                if (xml.name().toString() == sub_tag) {
                    auto jso = json::rename(readObject(xml), map_fields);
                    //fix some values
                    auto type = jso["type"].toString();
                    jso["type"] = (type == "Line" || type == "1") ? QString("track")
                                                                  : QString("direct");
                    auto actions = jso["actions"].toString();
                    if (actions.contains('=')) {
                        QJsonObject jso_actions;
                        for (auto s : actions.split(',', Qt::SkipEmptyParts)) {
                            auto kv = s.split('=', Qt::SkipEmptyParts);
                            if (kv.size() != 2 || kv.at(0).isEmpty())
                                continue;
                            jso_actions[kv.at(0)] = kv.at(1);
                        }
                        jso["actions"] = jso_actions;
                    } else {
                        jso.remove("actions");
                    }
                    // fix lat/lon numbers format str->real
                    if (!jso.contains("lat") || !jso.contains("lon")) {
                        qWarning() << "missing lat/lon" << jso;
                        continue;
                    }

                    // continue to next point
                    jsa.push_back(json::filter_names(jso));
                    record_count++;
                    continue;
                }
                xml.skipCurrentElement();
            }
            if (!jsa.isEmpty())
                jso_mission[name] = jsa;
            continue;
        }

        xml.skipCurrentElement();
    }

    if (!record_count) {
        qWarning() << "empty mission" << jso_import;
        return {};
    }

    jso_mission["import"] = jso_import;

    return json::fix_numbers(json::filter_names(jso_mission));
}