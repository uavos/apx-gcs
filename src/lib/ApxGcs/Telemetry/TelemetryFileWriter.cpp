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
#include "TelemetryFileWriter.h"
#include "TelemetryFileFormat.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <Vehicles/Vehicle.h>

#include <TelemetryValuePack.h>
#include <TelemetryValueUnpack.h>

#define dump(s, p, n) qDebug() << s << QByteArray((const char *) p, n).toHex().toUpper()

using namespace telemetry;

TelemetryFileWriter::TelemetryFileWriter(QObject *parent)
    : QFile(parent)
{}

TelemetryFileWriter::TelemetryFileWriter(const Fields &fields, QObject *parent)
    : QFile(parent)
    , _fields(fields)
{}

TelemetryFileWriter::~TelemetryFileWriter()
{
    if (_lock_file)
        delete _lock_file;
}

QLockFile *TelemetryFileWriter::get_lock_file(QString fileName)
{
    fileName = QFileInfo(fileName).fileName();
    auto lock_file = new QLockFile(QDir::temp().absoluteFilePath(fileName.append(".lock")));
    lock_file->setStaleLockTime(0);
    if (!lock_file->tryLock(10)) {
        qWarning() << "failed to lock file" << fileName;
        delete lock_file;
        return nullptr;
    }
    return lock_file;
}

void TelemetryFileWriter::close()
{
    QFile::close();
    if (_lock_file) {
        delete _lock_file;
        _lock_file = nullptr;
    }
}

bool TelemetryFileWriter::create(const QString &path, quint64 time_utc, QJsonObject info)
{
    if (isOpen()) {
        qDebug() << "file break";
        close();
    }

    setFileName(path);

    _lock_file = get_lock_file(fileName());
    if (!_lock_file)
        return true;

    // open file for writing
    if (!QFile::open(QIODevice::WriteOnly)) {
        qWarning() << "failed to open file" << fileName();
        close();
        return false;
    }

    // write file header
    fhdr_s fhdr{};
    strcpy(fhdr.magic, APXTLM_MAGIC);
    fhdr.version = APXTLM_VERSION;
    fhdr.payload_offset = sizeof(fhdr);
    fhdr.timestamp = time_utc;
    fhdr.utc_offset = QDateTime::fromMSecsSinceEpoch(time_utc).offsetFromUtc();
    write((const char *) &fhdr, sizeof(fhdr));

    // write info
    info["name_create"] = name(); // store original creation name
    write_meta("info", info);

    flush();

    // reset stream counters
    _widx = 0;
    _fields_map.clear();
    _values_s.clear();
    _stats_values.clear();
    _meta_objects.clear();
    _ts_s = 0;

    return true;
}

QString TelemetryFileWriter::prepare_file_name(QDateTime timestamp,
                                               const QString &callsign,
                                               QString dirPath)
{
    QString basename;
    {
        QStringList st;

        // add human readable timestamp
        auto ts = timestamp.toString("yyMMdd_HHmm_sszzz");
        auto utc_offset = timestamp.offsetFromUtc();
        ts.append(utc_offset > 0 ? 'E' : 'W');
        auto offset_mins = std::abs(utc_offset) / 60;
        ts.append(QString::number(offset_mins % 60 ? offset_mins : offset_mins / 60));
        st.append(ts);

        basename = st.join('_').toUpper();
    }

    {
        // fix callsign style
        auto cs = callsign.trimmed().toUpper().toUtf8();
        QString cs_fixed;
        for (auto s = cs.data(); *s; ++s) {
            auto c = *s;
            if (c >= '0' && c <= '9') { // allow any numbers
                cs_fixed.append(c);
            } else if (c >= 'A' && c <= 'Z') { // allow capital letters
                cs_fixed.append(c);
                continue;
            }
        }
        basename.append('-').append(cs_fixed.isEmpty() ? "U" : cs_fixed);
    }

    if (dirPath.isEmpty()) {
        return basename.append('.').append(telemetry::APXTLM_FTYPE);
    }

    QString fileName;
    QDir dir(dirPath);
    for (int i = 0; i < 100; ++i) {
        QString s = basename;
        if (i > 0)
            s.append(QString("-%1").arg(i, 2, 10, QChar('0')));

        s.append('.').append(telemetry::APXTLM_FTYPE);

        if (!QFile::exists(dir.absoluteFilePath(s))) {
            fileName = s;
            break;
        }
    }
    if (fileName.isEmpty()) {
        qWarning() << "failed to create file name";
        return {};
    }
    return fileName;
}

QJsonObject TelemetryFileWriter::get_info_from_filename(const QString &fileName)
{
    QJsonObject info;

    auto name = QFileInfo(fileName).completeBaseName();
    info["name"] = name;
    info["path"] = QFileInfo(fileName).absoluteFilePath();

    auto ts_s = name.section('-', 0, 0);
    auto ts_parts = ts_s.split('_');
    if (ts_parts.size() != 3) {
        qWarning() << "invalid file name" << fileName;
        return {};
    }

    // parse timestamp
    auto ts_fmt = QString("yyMMdd_HHmm_sszzz");
    auto s = ts_s.left(ts_fmt.size());
    auto timestamp = QDateTime::fromString(s, ts_fmt);
    if (!timestamp.isValid()) {
        qWarning() << "invalid timestamp" << s;
        return {};
    }

    s = ts_s.mid(ts_fmt.size());
    auto utc_offset_sign = s.startsWith('E') ? 1 : s.startsWith('W') ? -1 : 0;
    if (utc_offset_sign != 0) {
        auto offset_mins = s.mid(1).toInt();
        if (offset_mins <= 12)
            offset_mins *= 60;
        timestamp.setOffsetFromUtc(offset_mins * 60 * utc_offset_sign);
    } else {
        qWarning() << "invalid utc offset sign" << s;
    }
    info["timestamp"] = timestamp.toMSecsSinceEpoch();
    info["utc_offset"] = timestamp.offsetFromUtc();

    auto cs = name.section('-', 1, 1);
    if (cs.isEmpty()) {
        qWarning() << "invalid callsign" << name;
    } else {
        info["callsign"] = cs.section('-', 0, 0);
    }

    return info;
}

void TelemetryFileWriter::write_timestamp(quint32 timestamp_ms)
{
    if (!isOpen())
        return;

    // qDebug() << timestamp_ms << _ts_s;

    if (timestamp_ms <= _ts_s)
        return; // don't write repetitive timestamps
    _ts_s = timestamp_ms;

    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::ts};
    QFile::write((const char *) &dspec, 1);

    QFile::write((const char *) &timestamp_ms, 4);
}

void TelemetryFileWriter::write_values(quint32 timestamp_ms, const Values &values, bool uplink)
{
    if (!isOpen())
        return;

    write_timestamp(timestamp_ms);

    for (auto [uid, value] : values) {
        write_value(uid, value, uplink);
    }
    flush();
}

void TelemetryFileWriter::write_evt(quint32 timestamp_ms,
                                    const QString &name,
                                    const QString &value,
                                    const QString &uid,
                                    bool uplink)
{
    if (!isOpen())
        return;

    if (name.size() >= MAX_STRLEN || value.size() >= MAX_STRLEN || uid.size() >= MAX_STRLEN) {
        qWarning() << "string too long";
        return;
    }

    write_timestamp(timestamp_ms);

    if (uplink)
        _write_uplink();

    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::evt};
    QFile::write((const char *) &dspec, 1);

    _write_string(name.toUtf8());
    _write_string(value.toUtf8());
    _write_string(uid.toUtf8());
}

void TelemetryFileWriter::write_msg(quint32 timestamp_ms,
                                    const QString &text,
                                    const QString &subsystem)
{
    if (!isOpen())
        return;

    if (text.size() >= MAX_STRLEN || subsystem.size() >= MAX_STRLEN) {
        qWarning() << "string too long";
        return;
    }

    write_timestamp(timestamp_ms);

    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::msg};
    QFile::write((const char *) &dspec, 1);

    _write_string(text.toUtf8());
    _write_string(subsystem.toUtf8());
}

void TelemetryFileWriter::write_meta(const QString &name, const QJsonObject &data, bool uplink)
{
    if (!isOpen())
        return;

    if (data.isEmpty())
        return;

    if (name.size() >= MAX_STRLEN) {
        qWarning() << "string too long";
        return;
    }

    if (uplink)
        _write_uplink();

    QByteArray jdata;

    // check if file already written
    auto it = _meta_objects.find(name);
    if (it != _meta_objects.end()) {
        QJsonObject diff;
        json_diff(it->second, data, diff);
        if (diff.isEmpty()) {
            qDebug() << name << "json diff is empty";
            return;
        }
        _meta_objects[name] = data;
        jdata = QJsonDocument(diff).toJson(QJsonDocument::Compact);
        // qDebug() << name << "json diff:" << diff;
    } else {
        _meta_objects[name] = data;
        jdata = QJsonDocument(data).toJson(QJsonDocument::Compact);
        // qDebug() << name << "json:" << data;
    }

    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::meta};
    QFile::write((const char *) &dspec, 1);

    _write_string(name.toUtf8());

    auto ba = qCompress(jdata, 9);
    uint32_t size = ba.size();
    QFile::write((const char *) &size, 4);
    QFile::write(ba.constData(), size);
}

void TelemetryFileWriter::write_raw(quint32 timestamp_ms,
                                    uint16_t id,
                                    const QByteArray &data,
                                    bool uplink)
{
    if (!isOpen())
        return;

    write_timestamp(timestamp_ms);

    if (uplink)
        _write_uplink();

    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::raw};
    QFile::write((const char *) &dspec, 1);

    QFile::write((const char *) &id, 2);

    uint16_t size = data.size();
    QFile::write((const char *) &size, 2);
    QFile::write(data.constData(), data.size());
}

void TelemetryFileWriter::_write_string(const char *s)
{
    auto sz = qstrlen(s) + 1;
    if (sz > MAX_STRLEN) {
        qWarning() << "string too long" << sz;
        QFile::write(s, MAX_STRLEN - 1);
        QFile::write("\0", 1);
        return;
    }
    QFile::write(s, sz);
}

void TelemetryFileWriter::_write_field(QString name, QString title, QString units)
{
    // qDebug() << name << title << units;

    // write specifier
    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::field};
    QFile::write((const char *) &dspec, 1);

    // write field data
    _write_string(name.toUtf8());
    _write_string(title.toUtf8());
    _write_string(units.toUtf8());
}

void TelemetryFileWriter::write_value(mandala::uid_t uid, QVariant value, bool uplink)
{
    // map mandala fact
    auto f_it = _fields.find(uid);
    if (f_it == _fields.end()) {
        qWarning() << "unknown field" << uid;
        return;
    }
    const auto &field = f_it.value();

    // determine data type
    bool is_uint = field.dspec <= dspec_e::u64 || value.isNull();
    bool is_conv = !is_uint && (field.dspec == dspec_e::a16 || field.dspec == dspec_e::a32);

    if (!is_uint && !is_conv) {
        auto t = value.typeId();
        is_uint = t == QMetaType::Int || t == QMetaType::UInt || t == QMetaType::ULongLong;

        if (!is_uint) {
            // try check if float is integer
            auto vf = value.toDouble();
            if (std::isnan(vf) || std::isinf(vf) || vf == 0.) {
                is_uint = true;
                value = 0;
            } else if (vf > 0) {
                auto vfc = std::ceil(vf);
                if (vfc == vf) {
                    is_uint = true;
                    value = vfc;
                }
            }
        }
    }

    // uplink is always written
    if (!uplink) {
        // downlink is written only if value changed
        auto it = _values_s.find(uid);
        if (it != _values_s.end()) {
            if (it->second.typeId() == value.typeId() && it->second == value)
                return;
        } else {
            // value never posted before
            // initially - all values are assumed to be zero
            if (is_uint) {
                if (value.toULongLong() == 0)
                    return;
            } else {
                if (value.toFloat() == 0)
                    return;
            }
        }

        _values_s[uid] = value;
    }

    // map value index by UID and write field descriptor when needed
    uint16_t vidx;
    auto it = _fields_map.find(uid);
    if (it == _fields_map.end()) {
        if (_fields_map.size() >= 0x7FF) { // 11 bits
            // too many fields;
            return;
        }
        vidx = _fields_map.size();
        _fields_map[uid] = vidx;
        _write_field(field.name, field.title, field.units);
    } else {
        vidx = it->second;
    }

    // prepare specifier
    dspec_s spec{};
    if (vidx > _widx && (vidx - _widx) <= 8) {
        // use short form 8 bits specifier
        spec.spec8.opt8 = 1;
        spec.spec8.vidx_delta = (vidx - _widx) - 1;

    } else {
        // use 16 bits for specifier
        spec.spec16.vidx = vidx;
    }
    _widx = vidx;

    // prepare value according to format
    uint64_t wraw = 0;
    size_t wcnt = 0;

    if (is_uint) {
        wraw = value.toULongLong();
        // qDebug() << "uint" << v;

        if (wraw > 0xFFFFFFFF) {
            spec.spec8.dspec = dspec_e::u64;
            wcnt = 8;
        } else if (wraw > 0xFFFFFF) {
            spec.spec8.dspec = dspec_e::u32;
            wcnt = 4;
        } else if (wraw > 0xFFFF) {
            spec.spec8.dspec = dspec_e::u24;
            wcnt = 3;
        } else if (wraw > 0xFF) {
            spec.spec8.dspec = dspec_e::u16;
            wcnt = 2;
        } else if (wraw > 0) {
            spec.spec8.dspec = dspec_e::u8;
            wcnt = 1;
        }
    } else {
        // float types
        // qDebug() << "float" << value;
        spec.spec8.dspec = field.dspec;

        switch (field.dspec) {
        default:
            break;

        case telemetry::dspec_e::f16:
            wcnt = 2;
            wraw = xbus::telemetry::float_to_f16(value.toFloat());
            break;
        case telemetry::dspec_e::f32: {
            // check if f16 is enough
            const float vf = value.toFloat();
            uint16_t f16 = xbus::telemetry::float_to_f16(vf);
            if (xbus::telemetry::float_from_f16(f16) == vf) {
                spec.spec8.dspec = dspec_e::f16;
                wcnt = 2;
                wraw = f16;
                // qDebug() << "f16" << vf;
            } else {
                // store f32
                spec.spec8.dspec = dspec_e::f32;
                wcnt = 4;
                memcpy(&wraw, &vf, 4);
            }
            break;
        }
        case telemetry::dspec_e::f64: {
            wcnt = 8;
            double d = value.toDouble();
            memcpy(&wraw, &d, 8);
            break;
        }
        case telemetry::dspec_e::a16:
            wcnt = 2;
            wraw = (uint16_t) xbus::telemetry::float_to_angle(value.toFloat(), 180.f);
            break;
        case telemetry::dspec_e::a32:
            wcnt = 4;
            wraw = mandala::to_gps(value.toDouble());
            break;
        }
    }

    if (!wcnt) {
        spec.spec8.dspec = dspec_e::null;
    }

    // collect stats
    _stats_values[uid].insert(spec.spec8.dspec);

    // prepend uplink wrap when needed
    if (uplink)
        _write_uplink();

    // write specifier to file
    QFile::write((const char *) &spec, spec.spec8.opt8 ? 1 : 2);
    // dump("spec", &spec, spec.spec8.opt8 ? 1 : 2);

    // write payload (when not null)
    if (wcnt > 0) {
        QFile::write((const char *) &wraw, wcnt);
        // dump("data", &v, wcnt);
    }
}

void TelemetryFileWriter::_write_uplink()
{
    const dspec_s spec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::uplink};
    QFile::write((const char *) &spec, 1);
}

void TelemetryFileWriter::json_diff(const QJsonObject &prev,
                                    const QJsonObject &next,
                                    QJsonObject &diff)
{
    for (auto it = next.begin(); it != next.end(); ++it) {
        auto pit = prev.find(it.key());
        if (pit == prev.end()) {
            // new key added
            diff[it.key()] = it.value();
            // qDebug() << "new key" << it.key() << it.value();
        } else if (pit.value() != it.value()) {
            // qDebug() << "value diff" << it.key() << pit.value() << it.value();
            if (pit.value().isObject() && it.value().isObject()) {
                QJsonObject d;
                json_diff(pit.value().toObject(), it.value().toObject(), d);
                diff[it.key()] = d;
            } else if (pit.value().isArray() && it.value().isArray()) {
                QJsonArray da;
                auto pa = pit.value().toArray();
                auto na = it.value().toArray();
                for (int i = 0; i < na.size(); ++i) {
                    QJsonObject d;
                    json_diff(pa.at(i).toObject(), na.at(i).toObject(), d);
                    da.append(d);
                }
                diff[it.key()] = da;
            } else {
                diff[it.key()] = it.value();
            }
        }
    }
}

void TelemetryFileWriter::print_stats()
{
    if (_stats_values.empty())
        return;

    for (auto [uid, dspec] : _stats_values) {
        const auto &field = _fields[uid];
        QStringList sl;
        for (auto i : dspec)
            sl.append(telemetry::dspec_names[(uint) i]);

        qDebug() << field.name + ":" << sl.join(',');
    }
}
