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
#include "PApxTelemetry.h"

#include <App/AppLog.h>
#include <Mandala/Mandala.h>

#include <xbus/telemetry/TelemetryValuePack.h>

PApxTelemetry::PApxTelemetry(PApxVehicle *parent)
    : PTelemetry(parent)
    , _vehicle(parent)
    , _req(parent)
{
    connect(this, &Fact::enabledChanged, this, &PApxTelemetry::updateStatus);
    connect(this, &Fact::triggered, this, [this]() { decoder.reset(); });
}

void PApxTelemetry::updateStatus()
{
    if (!enabled()) {
        setValue(QString("RESYNC %1").arg(decoder.fmt_cnt()));
        return;
    }
    setValue(QString("%1 slots, %2 Hz").arg(decoder.slots_cnt()).arg(1000.0 / _dt_ms, 0, 'f', 1));
}

void PApxTelemetry::report()
{
    qDebug() << "----------------------------------";
    qDebug() << "decoder slots";
    qDebug() << "----------------------------------";

    for (size_t i = 0; i < decoder.slots_cnt(); ++i) {
        xbus::telemetry::fmt_e fmt = decoder.dec_slots().fields[i].fmt;
        QString path = Mandala::meta(decoder.dec_slots().fields[i].pid.uid).path;
        QString sfmt = QString::number(fmt);
        switch (fmt) {
        default:
            break;
        case xbus::telemetry::fmt_bit:
            sfmt = "bit";
            break;
        case xbus::telemetry::fmt_opt:
            sfmt = "opt";
            break;
        case xbus::telemetry::fmt_none:
            sfmt = "none";
            break;
        }
        qDebug() << i << sfmt << path;
    }
}

bool PApxTelemetry::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    // telemetry section uid
    switch (pid.uid) {
    default:
        return false;
    case mandala::cmd::env::telemetry::format::uid:
        trace()->data(stream.payload());
        if (pid.pri == xbus::pri_response) {
            if (stream.available() < (sizeof(uint8_t) * 2))
                break;
            uint8_t part, parts;
            stream >> part;
            stream >> parts;
            _request_format_part = 0;
            qDebug() << "format:" << part << parts << stream.available();
            if (!decoder.decode_format(part, parts, stream)) {
                qWarning() << decoder.valid() << decoder.slots_cnt();
                break;
            }

            if (decoder.valid()) {
                // decoder not yet valid
                return true;
            }
            if (parts <= 1) {
                // some already available parts are downloaded
                return true;
            }

            if (++part >= parts) {
                // extra parts error?
                qWarning() << "format done";
                _request_format_part = 0;
                break;
            }
            _request_format_part = part;
            request_format(part);
        }
        return true; // anyway accept the packet

    case mandala::cmd::env::telemetry::data::uid: // telemetry data stream
        if (stream.available() < xbus::telemetry::stream_s::psize()) {
            qWarning() << stream.available();
            break;
        }
        trace()->data(stream.toByteArray(stream.pos(), 4));     // ts
        trace()->data(stream.toByteArray(stream.pos() + 4, 1)); // hash
        trace()->data(stream.toByteArray(stream.pos() + 5, 1)); // fmt
        trace()->data(stream.toByteArray(stream.pos() + 6, stream.available() - 6));

        if (!unpack(pid.seq, stream))
            break;

        return true;
    }
    //error
    trace()->block("ERR:");
    trace()->data(stream.payload());

    _vehicle->incErrcnt();
    return true;

    //qDebug() << decoder.fmt_cnt() << QString::number(stream.ptr()[3], 16) << cobs.size();
}

bool PApxTelemetry::unpack(uint8_t pseq, PStreamReader &stream)
{
    bool upd = decoder.decode(pseq, stream);
    bool valid = decoder.valid();

    // manage timestamp wraps
    uint32_t seq = decoder.seq();
    uint32_t dt_ms = decoder.dt_ms();
    uint32_t dseq = (seq - _seq_s) & 0x7FFFFFFF;

    bool dt_ok = _dt_ms == dt_ms;
    _dt_ms = dt_ms;

    bool seq_ok = seq > _seq_s;
    _seq_s = seq;

    qint64 elapsed = _ts_time.isValid() ? _ts_time.elapsed() : 0;
    _ts_time.start();

    if (!dt_ok || !seq_ok || abs(elapsed - dseq * dt_ms) > (10 * 1000)) {
        _seq = 0;
        _timestamp_ms = 0;
    } else {
        _seq += dseq;
        _timestamp_ms = _seq * dt_ms;
    }

    //qDebug() << decoder.seq();

    // report and validate
    if (enabled() && !valid) {
        apxMsgW() << tr("Telemetry stream reset");
    }

    if (!enabled() && valid) {
        apxMsg() << tr("Telemetry stream valid");
        // TelemetryFormat format;
        // for (size_t i = 0; i < decoder.slots_cnt(); ++i) {
        //     format.append(decoder.dec_slots().fields[i].pid);
        // }
        // emit formatUpdated(format);
    }

    setEnabled(valid);

    if (!valid || !dt_ok) {
        updateStatus();
    }

    //qDebug() << valid << upd;

    if (!valid) {
        if (!_request_format_time.isValid() || _request_format_time.elapsed() > 500)
            request_format(_request_format_part);
        return true;
    }

    if (!upd)
        return true;

    // collect updated values
    PBase::Values values;

    for (size_t i = 0; i < decoder.slots_cnt(); ++i) {
        auto &flags = decoder.dec_slots().flags[i];
        if (!flags.upd)
            continue;
        flags.upd = false;
        auto const &f = decoder.dec_slots().fields[i];
        auto const &value = decoder.dec_slots().value[i];
        values.insert(f.pid.uid, raw_value(&value, flags.type));
    }

    emit telemetryData(values, _timestamp_ms);
    return true;
}

QVariant PApxTelemetry::raw_value(const void *src, mandala::type_id_e type)
{
    switch (type) {
    default:
        return QVariant();
    case mandala::type_real:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::real_t>(src, type));
    case mandala::type_dword:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::dword_t>(src, type));
    case mandala::type_word:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::word_t>(src, type));
    case mandala::type_byte:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::byte_t>(src, type));
    case mandala::type_option:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::option_t>(src, type));
    }
}

void PApxTelemetry::request_format(uint8_t part)
{
    //return;
    //qDebug() << part;
    _request_format_time.start();
    _req.request(mandala::cmd::env::telemetry::format::uid);
    _req << part;
    _req.send();
}
