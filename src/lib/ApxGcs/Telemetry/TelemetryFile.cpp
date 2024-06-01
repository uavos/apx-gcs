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
#include "TelemetryFile.h"
#include "TelemetryFileFormat.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <Vehicles/Vehicle.h>

#include <TelemetryValuePack.h>
#include <TelemetryValueUnpack.h>

#define dump(s, p, n) qDebug() << s << QByteArray((const char *) p, n).toHex().toUpper()

using namespace telemetry;

TelemetryFile::TelemetryFile() {}

bool TelemetryFile::create(Vehicle *vehicle)
{
    _vehicle = vehicle;

    if (isOpen()) {
        qDebug() << "file break";
        close();
    }

    auto dir = AppDirs::telemetry();
    dir.mkpath(".");

    QStringList st;

    auto t = QDateTime::currentDateTime();

    quint64 num = t.toMSecsSinceEpoch();

    st.append(QString::number(num));

    // st.append(t.toString("yyMMddHHmm"));

    QString callsign = vehicle->title();
    if (callsign.isEmpty())
        callsign = vehicle->confTitle();
    if (callsign.isEmpty())
        callsign = "U";

    st.append(callsign);

    QString fname;
    for (int i = 0; i < 100; ++i) {
        QString s = st.join('_');
        if (i > 0)
            s.append(QString("_%1").arg(i, 2, 10, QChar('0')));

        s.append('.').append(suffix);

        if (!QFile::exists(dir.absoluteFilePath(s))) {
            fname = s;
            break;
        }
    }

    if (fname.isEmpty()) {
        qWarning() << "failed to create file name";
        return false;
    }

    setFileName(dir.absoluteFilePath(fname));

    // open file for writing
    if (!open(QIODevice::WriteOnly)) {
        qWarning() << "failed to open file" << fileName();
        return false;
    }

    // write file header
    fhdr_s fhdr{};

    strcpy(fhdr.magic.magic, "APXTLM");
    fhdr.magic.version = version;
    fhdr.info.time = t.toMSecsSinceEpoch();
    fhdr.info.utc_offset = t.offsetFromUtc();

    // write tags
    XbusStreamWriter s(fhdr.tags, sizeof(fhdr.tags));
    _write_tag(&s, "call", vehicle->title().toUtf8());
    _write_tag(&s, "vuid", vehicle->uid().toUtf8());
    _write_tag(&s, "conf", vehicle->confTitle().toUtf8());
    _write_tag(&s, "class", vehicle->vehicleTypeText().toUtf8());
    _write_tag(&s, "host", QString("%1@%2").arg(App::username()).arg(App::hostname()).toUtf8());
    _write_tag(&s, "huid", App::machineUID().toUtf8());

    // write header to file
    QFile::write((const char *) &fhdr, sizeof(fhdr));
    flush();

    // reset stream counters
    _widx = 0;
    _fields_map.clear();
    _values_s.clear();
    _ts_s = 0;

    // write initial mandala state
    PBase::Values values;
    for (auto f : vehicle->f_mandala->valueFacts()) {
        if (!(f->everReceived() || f->everSent()))
            continue;

        values.push_back({f->uid(), f->value()});
        // qDebug() << f->mpath() << f->value();
    }
    write_values(values, false);

    return true;
}

void TelemetryFile::write_timestamp(quint32 timestamp_ms)
{
    // qDebug() << timestamp_ms << _ts_s;

    if (timestamp_ms <= _ts_s)
        return; // don't write repetitive timestamps
    _ts_s = timestamp_ms;

    const dspec_s dspec{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::ts};
    QFile::write((const char *) &dspec, 1);

    QFile::write((const char *) &timestamp_ms, 4);
}

void TelemetryFile::write_values(const PBase::Values &values, bool uplink)
{
    for (auto [uid, value] : values) {
        _write_value(uid, value, uplink);
    }

    flush();
}

bool TelemetryFile::_write_tag(XbusStreamWriter *stream, const char *name, const char *value)
{
    if (!value || !value[0]) // skip empty values
        return true;

    auto spos = stream->pos();

    do {
        if (!stream->write_string(name))
            break;
        stream->reset(stream->pos() - 1);

        if (!stream->write_string(":"))
            break;
        stream->reset(stream->pos() - 1);

        if (!stream->write_string(value))
            break;

        return true;
    } while (0);

    stream->reset(spos);
    return false;
}

void TelemetryFile::_write_string(const char *s)
{
    QFile::write(s, qstrlen(s) + 1);
}

void TelemetryFile::_write_field(QString name, QString title, QString units)
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

void TelemetryFile::_write_value(mandala::uid_t uid, const QVariant &value, bool uplink)
{
    // map mandala fact
    auto f = _vehicle->f_mandala->fact(uid);
    if (!f)
        return;

    // determine data type
    bool is_uint;
    if (f->fmt().type_id == mandala::type_real) {
        auto t = value.typeId();
        is_uint = t == QMetaType::Int || t == QMetaType::UInt || t == QMetaType::ULongLong;

        if (!is_uint) {
            float vf = value.toFloat();
            is_uint = std::isnan(vf) || (vf >= 0.f && std::ceil(vf) == vf);
        }
    } else {
        is_uint = true;
    }

    // uplink is always written
    if (!uplink) {
        // downlink is written only if value changed
        auto it = _values_s.find(uid);
        if (it != _values_s.end()) {
            if (it->second == value)
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

    // map value index by UID
    uint16_t vidx;
    auto it = _fields_map.find(uid);
    if (it == _fields_map.end()) {
        vidx = _fields_map.size();
        _fields_map[uid] = vidx;
        _write_field(f->mpath(), f->title(), f->units());
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

    // prepare value
    uint64_t v = 0;
    size_t wcnt = 0;

    if (is_uint) {
        v = value.toULongLong();
        // qDebug() << "uint" << v;

        if (v > 0xFFFFFFFF) {
            spec.spec8.dspec = dspec_e::u64;
            wcnt = 8;
        } else if (v > 0xFFFFFF) {
            spec.spec8.dspec = dspec_e::u32;
            wcnt = 4;
        } else if (v > 0xFFFF) {
            spec.spec8.dspec = dspec_e::u24;
            wcnt = 3;
        } else if (v > 0xFF) {
            spec.spec8.dspec = dspec_e::u16;
            wcnt = 2;
        } else if (v > 0) {
            spec.spec8.dspec = dspec_e::u8;
            wcnt = 1;
        } else {
            spec.spec8.dspec = dspec_e::null;
        }
    } else {
        // qDebug() << "float" << value;
        const float vf = value.toFloat();
        auto fmt = f->fmt();

        switch (fmt.fmt) {
        default:
            break;

        case mandala::fmt_u32:
            if (f->is_gps_converted()) {
                spec.spec8.dspec = dspec_e::a32;
                wcnt = 4;
                v = mandala::to_gps(value.toDouble());
            }
            break;

        case mandala::fmt_s16_rad:
        case mandala::fmt_s16_rad2:
            spec.spec8.dspec = dspec_e::a16;
            wcnt = 2;
            v = (uint16_t) xbus::telemetry::float_to_angle(vf, 180.f);
            break;
        }

        // default to raw float32
        if (!wcnt) {
            // check if f16 is enough
            uint16_t f16 = xbus::telemetry::float_to_f16(vf);
            if (xbus::telemetry::float_from_f16(f16) == vf) {
                spec.spec8.dspec = dspec_e::f16;
                wcnt = 2;
                v = f16;
                // qDebug() << "f16" << vf;
            } else {
                // store f32
                spec.spec8.dspec = dspec_e::f32;
                wcnt = 4;
                memcpy(&v, &vf, 4);
            }
        }
    }

    // prepend uplink wrap when needed
    if (uplink) {
        const dspec_s spec_uplink{.spec_ext.dspec = dspec_e::ext, .spec_ext.extid = extid_e::uplink};
        QFile::write((const char *) &spec_uplink, 1);
        // dump("up", &spec_uplink, 1);
    }

    // write specifier to file
    QFile::write((const char *) &spec, spec.spec8.opt8 ? 1 : 2);
    // dump("spec", &spec, spec.spec8.opt8 ? 1 : 2);

    // write payload (when not null)
    if (wcnt > 0) {
        QFile::write((const char *) &v, wcnt);
        // dump("data", &v, wcnt);
    }
}
