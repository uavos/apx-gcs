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

#include <TelemetryValuePack.h>
#include <TelemetryValueUnpack.h>

#include <ApxMisc/JsonHelpers.h>

#define dump(s, p, n) qDebug() << s << QByteArray((const char *) p, n).toHex().toUpper()

using namespace telemetry;

TelemetryFileWriter::TelemetryFileWriter()
    : QDataStream()
{}

TelemetryFileWriter::~TelemetryFileWriter()
{
    if (_lock_file)
        delete _lock_file;
}

QLockFile *TelemetryFileWriter::get_lock_file(QString name)
{
    auto lock_file = new QLockFile(QDir::temp().absoluteFilePath(name.append(".lock")));
    lock_file->setStaleLockTime(0);
    if (!lock_file->tryLock(10)) {
        qWarning() << "failed to lock file" << name;
        delete lock_file;
        return nullptr;
    }
    return lock_file;
}

telemetry::dspec_e TelemetryFileWriter::dspec_for_uid(const mandala::uid_t uid)
{
    mandala::type_id_e type_id = {};

    bool ok = false;
    for (const auto &m : mandala::meta) {
        if (m.uid != uid)
            continue;

        type_id = m.type_id;
        ok = true;
        break;
    }
    if (!ok)
        return telemetry::dspec_e::f32;

    // guess float types
    switch (mandala::fmt(uid).fmt) {
    default:
        break;
    case mandala::fmt_a32:
        return telemetry::dspec_e::a32;
    case mandala::fmt_s16_rad:
    case mandala::fmt_s16_rad2:
        return telemetry::dspec_e::a16;
    case mandala::fmt_f16:
    case mandala::fmt_s8:
    case mandala::fmt_s8_10:
    case mandala::fmt_u8_10:
    case mandala::fmt_s8_01:
    case mandala::fmt_s8_001:
    case mandala::fmt_u8_01:
    case mandala::fmt_u8_001:
    case mandala::fmt_u8_u:
    case mandala::fmt_s8_u:
    case mandala::fmt_s8_rad:
        return telemetry::dspec_e::f16;
    }

    switch (type_id) {
    default:
        break;
    case mandala::type_byte:
    case mandala::type_word:
    case mandala::type_dword:
        // uint types will be truncated by stream writer when needed
        return telemetry::dspec_e::u32;
    }

    return telemetry::dspec_e::f32;
}

void TelemetryFileWriter::flush()
{
    if (!isOpen())
        return;

    auto f = qobject_cast<QFile *>(device());
    if (f)
        f->flush();
}

void TelemetryFileWriter::close()
{
    if (!isOpen())
        return;

    auto f = qobject_cast<QFile *>(device());
    if (f)
        f->close();
    setDevice(nullptr);

    if (_lock_file) {
        delete _lock_file;
        _lock_file = nullptr;
    }
}

bool TelemetryFileWriter::init(QIODevice *d, const QString &name, quint64 time_utc, QJsonObject info)
{
    if (!d || !d->isOpen() || !d->isWritable())
        return false;

    _lock_file = get_lock_file(name);
    if (!_lock_file)
        return true;
    _name = name;

    // reset stream
    _widx = 0;
    _field_index.clear();
    _evt_index.clear();
    _values_s.clear();
    _stats_values.clear();
    _jso_s.clear();
    _ts_s = 0;
    _str_cache = {""}; // reserve empty string

    resetStatus();
    setDevice(d);

    // write file header
    fhdr_s fhdr{};
    strcpy(fhdr.magic, APXTLM_MAGIC);
    fhdr.version = APXTLM_VERSION;
    fhdr.payload_offset = sizeof(fhdr);
    fhdr.timestamp = time_utc;
    fhdr.utc_offset = QDateTime::fromMSecsSinceEpoch(time_utc).offsetFromUtc();
    if (!write(&fhdr, sizeof(fhdr)))
        return false;

    // write info
    info["title"] = name; // store original creation name
    write_jso(0, "info", info, false);

    flush();

    return true;
}

void TelemetryFileWriter::write_timestamp(quint32 timestamp_ms)
{
    if (!isOpen())
        return;

    // qDebug() << timestamp_ms << _ts_s;

    if (timestamp_ms == _ts_s)
        return; // don't write repetitive timestamps
    _ts_s = timestamp_ms;

    const dspec_s dspec{extid_e::ts};
    write(&dspec, 1);

    write(&timestamp_ms, 4);
}

void TelemetryFileWriter::write_values(quint32 timestamp_ms, const Values &values, bool dir)
{
    if (!isOpen())
        return;

    if (timestamp_ms > 0)
        write_timestamp(timestamp_ms);

    for (const auto [field, value] : values) {
        write_value(field, value, dir);
    }

    flush();
}

void TelemetryFileWriter::write_evt(quint32 timestamp_ms,
                                    const Event *evt,
                                    const QStringList &values,
                                    bool dir,
                                    uint skip_cache_cnt)
{
    if (!isOpen())
        return;

    // register evtid if not done yet
    if (values.size() > evt->info.size()) {
        qWarning() << "values size is greater than names size";
        return;
    }

    size_t evt_index;
    auto it = _evt_index.find(evt);
    if (it == _evt_index.end()) {
        // register new evt
        evt_index = _evt_index.size();
        if (evt_index >= 255) {
            qWarning() << "too many event ids";
            return;
        }
        _evt_index.insert({evt, evt_index});
        _write_reg(extid_e::evtid, evt->name, evt->info);
    } else {
        // evt is already registered
        evt_index = it->second;
    }

    // write event
    if (timestamp_ms > 0)
        write_timestamp(timestamp_ms);

    if (dir)
        _write_dir();

    const dspec_s dspec{extid_e::evt};
    write(&dspec, 1);
    write(&evt_index, 1);
    const auto icached = evt->info.size() - skip_cache_cnt;
    for (int i = 0; i < evt->info.size(); ++i) {
        auto value = i < values.size() ? values.at(i) : QString();
        _write_string_cached(value.toUtf8(), i >= icached);
    }
}

void TelemetryFileWriter::write_jso(quint32 timestamp_ms,
                                    const QString &name,
                                    const QJsonObject &data,
                                    bool dir)
{
    if (!isOpen())
        return;

    if (data.isEmpty())
        return;

    if (name.size() >= MAX_STRLEN) {
        qWarning() << "string too long";
        return;
    }

    QByteArray jdata;

    // check if file already written
    auto it = _jso_s.find(name);
    if (it != _jso_s.end()) {
        auto prev = it->second;
        auto diff = json::diff(prev, data);
        if (diff.isEmpty()) {
            qDebug() << name << "json diff is empty";
            return;
        }
        _jso_s[name] = data;
        jdata = QJsonDocument(diff).toJson(QJsonDocument::Compact);
        // json::save(name + "-diff", diff);
        // json::save(name + "-data", data);
        // json::save(name + "-test", json::merge(prev, diff));
    } else {
        _jso_s[name] = data;
        jdata = QJsonDocument(data).toJson(QJsonDocument::Compact);
        // json::save(name + "-orig", data);
    }

    if (timestamp_ms > 0)
        write_timestamp(timestamp_ms);

    if (dir)
        _write_dir();

    const dspec_s dspec{extid_e::jso};
    write(&dspec, 1);

    _write_string_cached(name.toUtf8());

    auto ba = qCompress(jdata, 9);
    uint32_t size = ba.size();
    write(&size, 4);
    write(ba.constData(), size);
}

void TelemetryFileWriter::write_raw(quint32 timestamp_ms,
                                    const QString &name,
                                    const QByteArray &data,
                                    bool dir)
{
    if (!isOpen())
        return;

    size_t size = data.size();

    QByteArray zip_data;
    if (size >= MIN_ZCOMP) {
        zip_data = qCompress(data, 9);
        size = zip_data.size();
        if (size > std::numeric_limits<uint16_t>::max()) {
            qWarning() << "compressed data too long" << size;
            return;
        }
    }

    if (timestamp_ms > 0)
        write_timestamp(timestamp_ms);

    if (dir)
        _write_dir();

    if (zip_data.size() > 0) {
        const dspec_s dspec{extid_e::zip};
        write(&dspec, 1);
        _write_string_cached(name.toUtf8());
        write(&size, 4);
        write(zip_data.constData(), size);
    } else {
        const dspec_s dspec{extid_e::raw};
        write(&dspec, 1);
        _write_string_cached(name.toUtf8());
        write(&size, 2);
        write(data.constData(), size);
    }
}

void TelemetryFileWriter::_write_string(const char *s)
{
    auto sz = qstrlen(s) + 1;
    if (sz > MAX_STRLEN) {
        qWarning() << "string too long" << sz;
        write(s, MAX_STRLEN - 1);
        write("\0", 1);
        return;
    }
    write(s, sz);
}

void TelemetryFileWriter::_write_string_cached(const char *s, bool skip_cache)
{
    // find string in cache
    auto it = std::find(_str_cache.begin(), _str_cache.end(), s);
    if (it != _str_cache.end()) {
        // string found in cache
        auto idx = std::distance(_str_cache.begin(), it);
        if (idx == 0) {
            // empty string
            write("\0", 1);
        } else {
            // write index
            write(&idx, 1);
        }
        return;
    }

    // string not found in cache
    auto sz = qstrlen(s) + 1;
    if (sz > MAX_STRLEN) {
        qWarning() << "string too long" << sz;
        write("\0", 1); // just write empty string instead
        return;
    }
    // add string to cache
    auto idx = _str_cache.size();
    if (idx >= 254 || skip_cache) {
        idx = 255; // string is not cacheable
    } else {
        _str_cache.push_back(s);
    }

    // qDebug() << "cache" << idx << s;

    // write cache index + string
    write((char *) &idx, 1);
    write(s, sz);
}

void TelemetryFileWriter::_write_reg(extid_e extid, QString name, QStringList stings)
{
    // qDebug() << name << strings;

    // write specifier
    const dspec_s dspec{extid};
    write(&dspec, 1);

    // write field data
    _write_string(name.toUtf8());

    size_t N = stings.size();
    write(&N, 1);
    for (auto &s : stings)
        _write_string(s.toUtf8());
}

void TelemetryFileWriter::write_value(const Field *field, const QVariant &value, bool dir)
{
    // determine data type
    auto dspec = field->dspec;
    bool is_uint = dspec <= dspec_e::u64 || value.isNull();
    bool is_conv = !is_uint && (dspec == dspec_e::a16 || dspec == dspec_e::a32);

    auto value_d = value.toDouble();
    auto value_u = value.toULongLong();

    if (!is_uint && !is_conv) {
        auto t = value.typeId();
        is_uint = t == QMetaType::Int || t == QMetaType::UInt || t == QMetaType::ULongLong;

        if (!is_uint) {
            // try check if float is integer
            if (std::isnan(value_d) || std::isinf(value_d) || value_d == 0.) {
                is_uint = true;
                value_u = 0;
                value_d = 0;
            } else if (value_d > 0) {
                auto vfc = std::ceil(value_d);
                if (vfc == value_d) {
                    is_uint = true;
                    value_u = vfc;
                }
            }
        }
    }

    // uplink is always written
    if (!dir) {
        // downlink is written only if value changed
        auto it = _values_s.find(field);
        if (it != _values_s.end()) {
            if (it->second == value_d)
                return;
            it->second = value_d;
        } else {
            // value never posted before
            // initially - all values are assumed to be zero
            if (is_uint) {
                if (value_u == 0)
                    return;
            } else {
                if (value_d == 0)
                    return;
            }
            _values_s.insert({field, value_d});
        }
    }

    // map value index by field index and write field descriptor when needed
    size_t field_index;
    auto it = _field_index.find(field);
    if (it == _field_index.end()) {
        // add new field to file
        field_index = _field_index.size();
        if (field_index >= 0x7FF) { // 11 bits
            qWarning() << "too many fields";
            return;
        }
        _field_index.insert({field, field_index});
        _write_reg(extid_e::field, field->name, field->info);
    } else {
        field_index = it->second;
    }

    // prepare specifier
    dspec_s spec{};
    const uint16_t widx = field_index;
    if (widx > _widx && (widx - _widx) <= 8) {
        // use short form 8 bits specifier
        spec.spec8.opt8 = 1;
        spec.spec8.vidx_delta = (widx - _widx) - 1;

    } else {
        // use 16 bits for specifier
        spec.spec16.vidx = widx;
    }
    _widx = widx;

    // prepare value according to format
    uint64_t wraw = 0;
    size_t wcnt = 0;

    if (is_uint) {
        wraw = value_u;
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
        spec.spec8.dspec = dspec;

        switch (dspec) {
        default:
            break;

        case telemetry::dspec_e::f16:
            wcnt = 2;
            wraw = xbus::telemetry::float_to_f16(value_d);
            break;
        case telemetry::dspec_e::f32: {
            // check if f16 is enough
            const float vf = value_d;
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
            memcpy(&wraw, &value_d, 8);
            break;
        }
        case telemetry::dspec_e::a16:
            wcnt = 2;
            wraw = (uint16_t) xbus::telemetry::float_to_angle(value_d, 180.f);
            break;
        case telemetry::dspec_e::a32:
            wcnt = 4;
            wraw = xbus::telemetry::deg_to_a32(value_d);
            break;
        }
    }

    if (!wcnt) {
        spec.spec8.dspec = dspec_e::null;
    }

    // collect stats
    _stats_values[field_index].insert(spec.spec8.dspec);

    // prepend dir wrap when needed
    if (dir)
        _write_dir();

    // write specifier to file
    write(&spec, spec.spec8.opt8 ? 1 : 2);
    // dump("spec", &spec, spec.spec8.opt8 ? 1 : 2);

    // write payload (when not null)
    if (wcnt > 0) {
        write(&wraw, wcnt);
        // dump("data", &v, wcnt);
    }
}

void TelemetryFileWriter::_write_dir()
{
    const dspec_s spec{extid_e::dir};
    write(&spec, 1);
}
