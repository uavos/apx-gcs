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
#include "TelemetryFileEvents.h"
#include "TelemetryFileReader.h"
#include "TelemetryFileWriter.h"

#include <App/App.h>
#include <App/AppDirs.h>

#include <ApxMisc/JsonHelpers.h>

#include <Mandala/MandalaAliases.h>
#include <XbusNode.h>

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
                QString format;
                if (xml.name().compare("telemetry") == 0) {
                    format = xml.attributes().value("format").toString();
                    if (format.isEmpty())
                        format = "old";
                } else if (xml.name().compare("telemetry.gcu.uavos.com") == 0) {
                    format = "v9";
                } else {
                    rv = false;
                    break;
                }

                rv = import_telemetry_xml(xml, format);
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

QJsonValue TelemetryFileImport::import_js(QXmlStreamReader &xml,
                                          const QStringList &object_tags,
                                          const QStringList &array_tags)
{
    QHash<QString, QJsonArray> arrays;
    QJsonObject jso;
    // read attributes
    for (const auto &attr : xml.attributes()) {
        jso[attr.name().toString()] = attr.value().toString();
    }

    // read elements
    while (xml.readNextStartElement()) {
        auto tag = xml.name().toString();

        if (object_tags.contains(tag)) {
            jso[tag] = import_js(xml, object_tags, array_tags);
            continue;
        }

        if (array_tags.contains(tag)) {
            auto name = xml.attributes().value("name").toString();
            auto jsa_item = import_js(xml, object_tags, array_tags).toObject();
            if (!name.isEmpty())
                jsa_item["name"] = name;
            arrays[tag].push_back(jsa_item);
            continue;
        }

        // simple tag
        jso[tag] = xml.readElementText(QXmlStreamReader::IncludeChildElements);
    }

    for (auto [tag, jsa] : arrays.asKeyValueRange())
        jso[tag] = jsa;

    return json::fix_numbers(json::remove_empty(jso));
}

bool TelemetryFileImport::import_telemetry_xml(QXmlStreamReader &xml, QString format)
{
    apxMsg() << tr("Importing XML").append(':') << QFileInfo(_srcFileName).fileName() << format;

    bool ok = true;

    while (ok) {
        //read header
        _info = {};
        QJsonObject &info = _info;

        std::map<QString, QJsonObject> missions;
        std::map<QString, QJsonObject> nodes;
        QStringList fields;

        // some default values
        QJsonObject jso_import;
        jso_import["name"] = QFileInfo(_srcFileName).fileName();
        jso_import["title"] = QFileInfo(_srcFileName).baseName();
        jso_import["hash"] = _src_hash;
        jso_import["format"] = format;

        bool format_v9 = format == "v9";
        if (format_v9) {
            const auto title = xml.attributes().value("title").toString().trimmed();
            if (!title.isEmpty())
                jso_import["title"] = title;
            const auto s_utc = xml.attributes().value("UTC").toString().trimmed();
            jso_import["timestamp"] = s_utc;
            const auto ts = QDateTime::fromString(s_utc, Qt::ISODate).toMSecsSinceEpoch();
            if (ts > 0) {
                info["timestamp"] = ts;
                // qDebug() << "timestamp from UTC:" << ts << s_utc;
            } else {
                apxMsgW() << tr("Invalid timestamp").append(':') << s_utc;
            }
            // get unit name from title v9
            auto unit_name = title.split('_').mid(6).join('_').trimmed();
            if (!unit_name.isEmpty()) {
                QJsonObject unit;
                unit["name"] = unit_name;
                info["unit"] = unit;
            }
        }

        bool data_begins = false;
        QString tag;
        while (xml.readNextStartElement()) {
            tag = xml.name().toString();

            if (format_v9) {
                if (tag == "mandala") {
                    auto jso = import_js(xml).toObject();
                    fields = jso["fields"].toString().split(',', Qt::SkipEmptyParts);
                    auto version = jso["version"].toString().trimmed();
                    if (!version.isEmpty()) {
                        QJsonObject sw;
                        sw["version"] = version;
                        info["sw"] = sw;
                    }
                    // the only tag to read in header for v9
                    data_begins = true;
                    break;
                }
                // tag not recognized for v9
                xml.skipCurrentElement();
                continue;
            }

            if (tag == "data") {
                data_begins = true;
                break; // stop on data
            }

            if (tag == "info") {
                auto jso = import_js(xml).toObject();
                info["timestamp"] = jso["time"].toVariant().toLongLong();
                info["conf"] = jso["comment"].toString();

                QJsonObject unit;
                unit["uid"] = jso["vehicleUID"].toString();
                unit["name"] = jso["callsign"].toString();
                info["unit"] = unit;
                continue;
            }

            if (tag == "user") {
                auto jso = import_js(xml).toObject();
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
                    jso = json::fix_numbers(json::remove_empty(jso));
                    if (err.error != QJsonParseError::NoError || jso.isEmpty()) {
                        apxMsgW() << err.errorString();
                        continue;
                    }
                    if (tag == "vehicle") {
                        // unit nodes config
                        nodes[hash] = jso;
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
        if (!data_begins || fields.isEmpty() || info.isEmpty()) {
            apxMsgW() << tr("XML data error");
            qDebug() << info;
            qDebug() << fields;
            ok = false;
            break;
        }

        // save import info
        jso_import = json::remove_empty(jso_import);
        info["import"] = jso_import;

        // parse title
        if (format_v9) {
            // file title encodes local timestamp
            // find UTC offset by comparing to UTC timestamp from tag
            auto st = jso_import["title"].toString().split('_');
            if (st.size() < 6) {
                apxMsgW() << tr("Invalid title format").append(':') << jso_import["title"];
            } else {
                auto ts_name = QDateTime::fromString(st.mid(0, 5).join('_'), "yyyy_MM_dd_HH_mm");
                ts_name.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(0));
                // qDebug() << "timestamp from name:" << ts_name.toMSecsSinceEpoch();
                if (!ts_name.isValid()) {
                    apxMsgW() << tr("Invalid timestamp in title").append(':') << st;
                }

                auto timestamp = info["timestamp"].toVariant().toULongLong();
                if (timestamp > 0) {
                    qint64 utc_offset = (ts_name.toMSecsSinceEpoch() - timestamp) / 1000;
                    // qDebug() << "utc_offset:" << utc_offset;

                    if (std::abs(utc_offset) > 24 * 3600 * 1000) {
                        apxMsgW() << tr("Invalid UTC offset").append(':') << utc_offset;
                        qWarning() << "utc_offset" << utc_offset << "ts_name"
                                   << ts_name.toMSecsSinceEpoch() << "timestamp" << timestamp;
                    } else {
                        info["utc_offset"] = utc_offset;
                        ts_name.setTimeZone(QTimeZone::fromSecondsAheadOfUtc(utc_offset));
                        // compare two timestamps
                        auto tms_name = ts_name.toMSecsSinceEpoch();
                        auto tms = info["timestamp"].toVariant().toULongLong();
                        if (tms_name != tms) {
                            apxMsgW() << tr("Timestamp mismatch").append(':') << tms_name << tms
                                      << utc_offset;
                            if (tms == 0) {
                                info["timestamp"] = tms_name;
                            }
                        } else {
                            // qDebug() << "utc_offset" << utc_offset << "tms_name" << tms_name
                            //          << "tms" << tms;
                        }
                    }
                } else if (ts_name.isValid()) {
                    // no timestamp in info, use ts_name
                    info["timestamp"] = ts_name.toMSecsSinceEpoch();
                    qDebug() << "set timestamp from title" << info["timestamp"];
                } else {
                    apxMsgW() << tr("Invalid timestamp").append(':') << jso_import["title"];
                    ok = false;
                    break;
                }
            }

        } else {
            auto s = jso_import["title"].toString();
            auto s_ts = s.left(s.indexOf('-'));
            auto s_name = s.mid(s.indexOf('-') + 1);
            auto ts_name = QDateTime::fromString(s_ts, "yyyy_MM_dd_HH_mm_ss_zzz");
            auto s_timestamp = jso_import["timestamp"].toVariant().toString();

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
                auto tms = info["timestamp"].toVariant().toULongLong();
                if (tms_name != tms) {
                    apxMsgW() << tr("Timestamp mismatch").append(':') << tms_name << tms
                              << utc_offset;
                    if (tms == 0) {
                        info["timestamp"] = tms_name;
                    }
                }
            }
        } // parse title

        auto timestamp = info["timestamp"].toVariant().toULongLong();
        if (timestamp == 0 || QDateTime::fromMSecsSinceEpoch(timestamp).date().year() < 2010) {
            apxMsgW() << tr("Unknown timestamp");
            qWarning() << info;
            ok = false;
            break;
        }

        // qDebug() << info;
        // qDebug() << fields;

        // map fields to current mandala for dspec config
        std::vector<TelemetryFileWriter::Field> fields_stream;
        for (auto f : fields) {
            auto it = mandala::ALIAS_MAP.find(f.toStdString());
            if (it != mandala::ALIAS_MAP.end()) {
                auto uid = it->second;
                auto dspec = TelemetryFileWriter::dspec_for_uid(uid);
                fields_stream.push_back({f, {}, dspec});
                continue;
            }
            // qDebug() << f;
            // push field as-is (old flat format)
            fields_stream.push_back({f, {}, telemetry::dspec_e::f32});
        }

        // init stream writer
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

            if (format_v9) {
            } else {
                // downlink data
                if (tag == "D") {
                    TelemetryFileWriter::Values values;
                    size_t i = 0;
                    for (const auto &s : xml.readElementText().split(',', Qt::KeepEmptyParts)) {
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
                    for (const auto &f : fields) {
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
                    if (evt_name.isEmpty()) {
                        qWarning() << "unknown event" << evt_name << value;
                        continue;
                    }
                    // qDebug() << "event" << evt_name << value << uid;
                    if (evt_name == "msg") {
                        auto msg = value;
                        auto src = QString();
                        if (msg.contains('[') && msg.contains(']')) {
                            auto i1 = msg.indexOf('[');
                            auto i2 = msg.indexOf(']');
                            src = msg.mid(i1 + 1, i2 - i1 - 1);
                            msg.remove(i1, i2 - i1 + 1);
                        }
                        stream.write_evt(time_tag, &telemetry::EVT_MSG, {uid, src, msg}, false, 1);
                        continue;
                    }
                    if (evt_name == "mission") {
                        auto it = missions.find(uid);
                        if (it == missions.end())
                            continue;
                        auto jso = it->second;
                        jso["import"] = json::merge(jso["import"].toObject(), jso_import);
                        stream.write_jso(time_tag, evt_name, jso, uplink);
                        continue;
                    }
                    if (evt_name == "nodes") {
                        auto it = nodes.find(uid);
                        if (it == nodes.end())
                            continue;
                        auto jso = it->second;
                        jso["import"] = jso_import;
                        // qDebug() << value << jso;
                        stream.write_jso(time_tag, evt_name, jso, uplink);
                        continue;
                    }

                    continue;
                } // <E> event
            }

            // special tags with objects

            if (tag == "mission") {
                auto jso = import_mission(xml);
                if (jso.isEmpty())
                    continue;
                jso["import"] = json::merge(jso["import"].toObject(), jso_import);
                if (!jso.contains("time"))
                    jso["time"] = (qint64) timestamp + time_tag;

                // qDebug() << tag << jso;
                stream.write_jso(time_tag, tag, jso, false);
                continue;
            }

            if (tag == "nodes") {
                auto jso = import_nodes(xml);
                jso["import"] = json::merge(jso["import"].toObject(), jso_import);
                if (!jso.contains("time"))
                    jso["time"] = (qint64) timestamp + time_tag;

                jso = json::fix_numbers(jso);
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

QJsonObject TelemetryFileImport::import_mission(QXmlStreamReader &xml)
{
    // already in <mission ...> tag
    if (xml.name().toString() != "mission") {
        qWarning() << "not a mission tag" << xml.name();
        return {};
    }

    static const QHash<QString, std::pair<QString, QString>> map_array{
        // tag, {sub_tag, name}
        {"waypoints", {"waypoint", "wp"}},
        {"points", {"point", "pi"}},
        {"runways", {"runway", "rw"}},
        {"taxiways", {"taxiway", "tw"}},
    };

    QStringList array_tags;
    QStringList object_tags{
        "info",
        "details",
        "home",
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

    // update maps
    for (auto [key, value] : map_array.asKeyValueRange()) {
        object_tags.push_back(key);
        array_tags.push_back(value.first);
        map_fields[value.first] = value.second;
    }

    const auto imp = import_js(xml, object_tags, array_tags).toObject();

    QJsonObject mission;
    mission["title"] = imp["info"]["title"];
    mission["time"] = imp["info"]["time"];

    {
        QJsonObject jso;
        jso["format"] = imp["format"];
        jso["title"] = imp["title"];
        jso["version"] = imp["version"];
        jso["timestamp"] = imp["timestamp"];
        jso["exported"] = imp["exported"];
        mission["import"] = jso;
    }

    size_t record_count = 0;
    for (auto [key, value] : map_array.asKeyValueRange()) {
        QJsonArray jsa;
        const auto name = value.second;
        for (const auto &jsv : imp[key][value.first].toArray()) {
            auto jso = json::rename(jsv.toObject(), map_fields);
            // fix some values
            jso.remove("id");
            jso.remove("idx");
            auto type = jso.take("type").toVariant().toString();
            if (name == "wp") {
                if (type == "Line" || type == "1") {
                    jso["xtrack"] = true;
                }
            } else if (name == "rw") {
                jso["type"] = (type == "Right" || type == "1") ? QString("right") : QString("left");
            }
            // actions are incompatible with old fmt, thus disabled for now
            jso.remove("actions");
            // fix lat/lon numbers format str->real
            if (!jso.contains("lat") || !jso.contains("lon")) {
                qWarning() << "missing lat/lon" << jso;
                continue;
            }

            // continue to next point
            jsa.push_back(json::remove_empty(jso));
            record_count++;
        }
        if (!jsa.isEmpty())
            mission[name] = jsa;
    }

    if (!record_count) {
        qWarning() << "empty mission" << mission;
        return {};
    }

    mission = json::fix_numbers(json::remove_empty(mission));
    // json::save("mission-conv", mission);
    // json::save("mission-orig", imp);

    return mission;
}

QJsonObject TelemetryFileImport::import_nodes(QXmlStreamReader &xml)
{
    // already in <nodes ...> tag
    if (xml.name().toString() != "nodes") {
        qWarning() << "not a nodes tag" << xml.name();
        return {};
    }

    const auto imp = import_js(xml,
                               {
                                   "dictionary",
                                   "config",
                                   "user",
                                   "commands",
                                   "fields",
                                   "struct",
                                   "info",
                                   "vehicle",
                                   "ident",
                                   "struct",
                               },
                               {
                                   "node",
                                   "command",
                                   "field",
                               })
                         .toObject();

    QJsonObject nodes;

    json::save("nodes-imp", imp);

    if (imp.contains("info")) {
        nodes["title"] = imp["info"]["title"];
        nodes["time"] = imp["info"]["time"];
    } else {
        nodes["title"] = imp["title"];
    }

    {
        QJsonObject jso;
        jso["format"] = imp["format"];
        jso["title"] = imp["title"];
        jso["version"] = imp["version"];
        jso["timestamp"] = imp["timestamp"];
        jso["exported"] = imp["exported"];
        nodes["import"] = jso;
    }
    {
        QJsonObject jso;
        const auto ident = imp[imp.contains("vehicle") ? "vehicle" : "ident"].toObject();
        jso["uid"] = ident["uid"];
        jso["name"] = ident["callsign"];
        jso["type"] = ident["class"];
        jso["time"] = ident["time"];
        nodes["unit"] = jso;
    }

    // nodes items
    QJsonArray nodes_array;
    for (const auto jsv : imp["node"].toArray()) {
        const auto imp_node = jsv.toObject();
        QJsonObject node;
        QJsonObject node_info;
        QJsonObject node_dict;

        node_info["uid"] = imp_node["sn"];
        node_info["name"] = imp_node["name"];

        if (imp_node.contains("info")) {
            const auto jso = imp_node["info"].toObject();
            node_info["version"] = jso["version"];
            node_info["hardware"] = jso["hardware"];
        } else if (imp_node.contains("dictionary")) {
            const auto jso = imp_node["dictionary"].toObject();
            node_info["version"] = jso["version"];
            node_info["hardware"] = jso["hardware"];
            node_dict["time"] = jso["time"];
        } else {
            node_info["version"] = imp["version"];
            node_info["hardware"] = imp["hardware"];
        }

        if (imp_node.contains("config")) {
            const auto jso = imp_node["config"].toObject();
            node["time"] = jso["time"];
            node["title"] = jso["title"];
        }

        if (imp_node.contains("user")) {
            const auto jso = imp_node["user"].toObject();
            QJsonObject host;
            host["hostname"] = jso["hostname"];
            host["username"] = jso["username"];
            host["uid"] = jso["machineUID"];
            node_info["host"] = host;
        }

        QJsonArray node_fields;
        QJsonObject node_values;

        // node commands
        const auto commands = imp_node.contains("commands")
                                  ? imp_node["commands"]["command"].toArray()
                                  : imp_node["command"].toArray();

        for (const auto &jsv : commands) {
            const auto jso = jsv.toObject();
            QJsonObject field;
            field["name"] = jso["name"];
            field["title"] = jso["descr"];
            field["type"] = "command";
            node_fields.push_back(field);
        }

        // node fields
        QStringList groups_index; // just to keep order
        node_fields = import_node_fields(imp_node["fields"]["field"].toArray(),
                                         &node_values,
                                         &groups_index,
                                         {});

        // remove default enum values
        for (const auto i : node_fields) {
            auto field = i.toObject();
            if (field["type"].toString() != "option")
                continue;
            auto opts = field["units"].toString().split(',');
            if (opts.size() < 2) {
                qWarning() << "empty opts" << field;
                continue;
            }
            const auto zero = opts.at(0);
            if (zero.isEmpty()) {
                qWarning() << "empty zero value" << field;
                continue;
            }
            const auto name = field["name"].toString();
            auto value = node_values[name];
            if (value.isArray()) {
                uint zcnt = 0;
                for (auto v : value.toArray()) {
                    if (v.toString() == zero)
                        zcnt++;
                }
                if (zcnt == value.toArray().size())
                    node_values.remove(name);
                continue;
            }
            if (value.toString() == zero)
                node_values.remove(name);
        }

        // zero or empty values (defaults) are removed
        node_values = json::remove_empty(json::fix_numbers(node_values), true);

        if (node_fields.isEmpty()) {
            qWarning() << "empty node" << node;
            continue;
        }

        // combine node parts
        node["info"] = node_info;
        node_dict["fields"] = node_fields;
        node["dict"] = node_dict;
        node["values"] = json::remove_empty(node_values, true);

        nodes_array.push_back(node);
    }

    if (nodes_array.isEmpty()) {
        qWarning() << "empty nodes" << imp;
        return {};
    }
    nodes["nodes"] = nodes_array;

    // clean up and return object
    nodes = json::fix_numbers(json::remove_empty(nodes));
    json::save("nodes-conv", nodes);
    // json::save("nodes-orig", imp);

    return nodes;
}

QJsonArray TelemetryFileImport::import_node_fields(const QJsonArray &src,
                                                   QJsonObject *values,
                                                   QStringList *groups_index,
                                                   QString name_prefix)
{
    QJsonArray node_fields;

    for (const auto &i : src) {
        const auto imp = i.toObject();

        // grouping
        auto sect = imp["sect"].toString();
        if (!sect.isEmpty()) {
            auto path_title = sect.split('/');
            auto path = path_title;
            for (auto &i : path) {
                i = i.toLower().replace(' ', '_').replace('-', '_').replace('.', '_');
            }
            // create missing group fields
            for (int i = 0; i < path.size(); i++) {
                auto group = path.mid(0, i + 1).join('.');
                if (groups_index->contains(group))
                    continue;
                groups_index->push_back(group);
                QJsonObject group_field;
                group_field["name"] = group;
                group_field["title"] = path_title.at(i);
                group_field["type"] = "group";
                node_fields.push_back(group_field);
            }
            sect = path.join('.') + '.';
        }

        QJsonObject field;
        field["name"] = name_prefix + sect + imp["name"].toString();
        field["title"] = imp["title"];
        field["descr"] = imp["descr"];

        auto value = imp["value"].toVariant().toString();
        auto type = imp["type"].toString();

        static const QHash<QString, xbus::node::conf::type_e> type_map{
            {"Float", xbus::node::conf::real},
            {"Byte", xbus::node::conf::byte},
            {"UInt", xbus::node::conf::dword},
            {"Option", xbus::node::conf::option},
            {"String", xbus::node::conf::string},
            {"StringL", xbus::node::conf::text},
            {"MandalaID", xbus::node::conf::bind},
            {"Script", xbus::node::conf::script},
            // {"Vector", xbus::node::conf::group},
            // {"Hash", xbus::node::conf::dword},
            // {"Array", xbus::node::conf::group},
        };

        if (type == "Array") {
            // first type of child field
            const auto jsa = imp["field"].toArray();
            const auto jso_item = jsa.first().toObject();
            type = jso_item["type"].toString();
            auto type_id = type_map.value(type);
            field["array"] = jsa.size();
            field["units"] = jso_item[type_id == xbus::node::conf::option ? "opts" : "units"];
            value.clear();
            QJsonArray jsa_values;
            for (const auto i : jsa) {
                auto v = i.toObject().value("value").toString();
                if (v.isEmpty())
                    continue;
                if (type_id == xbus::node::conf::bind)
                    v = import_mandala_bind(v);
                jsa_values.push_back(v);
            }
            if (!jsa_values.isEmpty())
                values->insert(field["name"].toString(), jsa_values);
        }

        if (type_map.contains(type)) {
            auto type_id = type_map.value(type);
            field["type"] = QString(xbus::node::conf::type_to_str(type_id));
            if (type_id == xbus::node::conf::option) {
                if (!imp["opts"].toString().isEmpty())
                    field["units"] = imp["opts"];
            } else if (type_id == xbus::node::conf::bind) {
                value = import_mandala_bind(value);
            } else {
                field["units"] = imp["units"];
            }
            node_fields.push_back(field);
            if (values && !value.isEmpty())
                values->insert(field["name"].toString(), value);
            continue;
        }
        if (type == "Vector" || type == "Hash") {
            field["type"] = "group";
            auto group = field["name"].toString();
            groups_index->push_back(group);
            node_fields.push_back(field);
            const auto sub = import_node_fields(imp["field"].toArray(),
                                                values,
                                                groups_index,
                                                group + '.');
            for (auto i : sub)
                node_fields.push_back(i.toObject());
            continue;
        }
        if (type == "Script") {
            continue;
        }
    }
    return node_fields;
}

QString TelemetryFileImport::import_mandala_bind(QString bind)
{
    if (bind.isEmpty())
        return bind;

    auto it = mandala::ALIAS_MAP.find(bind.toStdString());
    if (it == mandala::ALIAS_MAP.end())
        return bind;
    auto uid = it->second;
    // find mandala path for uid
    for (auto &i : mandala::meta) {
        if (i.uid != uid)
            continue;
        auto s = QString(i.path).split('.');
        if (s.size() != 4)
            return bind;
        // remove second part
        s.removeAt(1);
        return s.join('.');
    }
    return bind;
}
