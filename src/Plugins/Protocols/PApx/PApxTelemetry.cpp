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

#include <telemetry/TelemetryValuePack.h>

// TODO cache datasets in DB

PApxTelemetry::PApxTelemetry(PApxVehicle *parent)
    : PTelemetry(parent)
    , _req(parent)
{
    connect(this, &Fact::triggered, this, [this]() { decoder.reset(); });
}

void PApxTelemetry::updateStatus()
{
    QString s;
    if (!enabled()) {
        s = QString("RESYNC %1...").arg(_request_format_part);
    } else {
        s = QString("%1 slots, %2 Hz")
                .arg(decoder.slots_cnt())
                .arg(1000.0 / decoder.dt_ms(), 0, 'f', 1);
    }
    setValue(s);
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
        case xbus::telemetry::fmt_u4:
            sfmt = "opt";
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
    case mandala::cmd::env::telemetry::xpdr::uid:
        trace()->data(stream.payload());
        if (pid.pri == xbus::pri_request)
            return true;
        if (!unpack_xpdr(stream))
            break;
        return true;

    case mandala::cmd::env::telemetry::format::uid:
        trace()->data(stream.payload());
        if (pid.pri == xbus::pri_response) {
            _request_format_part = 0;

            xbus::telemetry::format_resp_hdr_s h{};
            if (!decoder.decode_format(stream, &h)) {
                qWarning() << decoder.valid() << decoder.slots_cnt() << h.part << h.pcnt
                           << stream.available();
                break;
            }
            //qDebug() << "format:" << h.part << h.total << stream.available();

            if (decoder.valid()) {
                // decoder not yet valid
                return true;
            }
            if (h.pcnt <= 1) {
                // some already available parts are downloaded
                return true;
            }

            if (++h.part >= h.pcnt) {
                // extra parts error?
                qWarning() << "format done" << h.part << h.pcnt;
                _request_format_part = 0;
                break;
            }
            _request_format_part = h.part;
            request_format(h.part);
        }
        return true; // anyway accept the packet

    case mandala::cmd::env::telemetry::data::uid: // telemetry data stream
        if (stream.available() < xbus::telemetry::hdr_s::psize()) {
            qWarning() << stream.available();
            break;
        }
        trace()->data(stream.toByteArray(stream.pos(), 2));     // ts
        trace()->data(stream.toByteArray(stream.pos() + 2, 1)); // hash
        trace()->data(stream.toByteArray(stream.pos() + 3, stream.available() - 3));

        if (!unpack(pid.seq, stream)) {
            _vehicle->setStreamType(PVehicle::DATA);
            break;
        }
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

    if (!_timer.isValid()) {
        decoder.reset_timestamp();
    } else if (decoder.timestamp_ms() > 0) {
        qint64 elapsed = _timer.elapsed() / 1000;
        qint64 dt = decoder.dt_ms() / 1000;
        qint64 latency_sec = elapsed > dt ? elapsed - dt : dt - elapsed;

        if (latency_sec > 10 * 60) {
            // Max 10 minutes latency.
            // Could be due to datalink queue delay.
            apxMsgW() << tr("Telemetry stream latency %1 sec for %2")
                             .arg(latency_sec)
                             .arg(parent()->title());
            decoder.reset_timestamp();
        }
    }
    _timer.start();

    auto timestamp = decoder.timestamp_ms();

    // qDebug() << timestamp;

    // report and validate
    if (enabled() && !valid) {
        apxMsgW() << tr("Telemetry stream reset for %1").arg(parent()->title());
        qDebug() << "ts" << timestamp;
        emit telemetryReset();
    }

    if (!enabled() && valid) {
        apxMsg() << tr("Telemetry stream valid from %1").arg(parent()->title());
        qDebug() << "valid"
                 << "ts" << timestamp;
    }

    bool update_status = enabled() != valid || !valid || !timestamp || _dt_ms != decoder.dt_ms();

    setEnabled(valid);
    _dt_ms = decoder.dt_ms();

    if (update_status) {
        updateStatus();
    }

    if (valid && !timestamp) {
        apxMsg() << tr("Telemetry stream timestamp reset from %1").arg(parent()->title());
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

    emit telemetryData(values, timestamp);
    return true;
}

QVariant PApxTelemetry::raw_value(const void *src, mandala::type_id_e type)
{
    switch (type) {
    case mandala::type_byte:
        return QVariant::fromValue((uint) xbus::telemetry::raw_value<mandala::byte_t>(src, type));
    case mandala::type_word:
        return QVariant::fromValue((uint) xbus::telemetry::raw_value<mandala::word_t>(src, type));
    case mandala::type_dword:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::dword_t>(src, type));
    case mandala::type_real:
        return QVariant::fromValue(xbus::telemetry::raw_value<mandala::real_t>(src, type));
    }
    return QVariant();
}

void PApxTelemetry::request_format(uint8_t part)
{
    // return;
    // when requests are disabled - the format is received silently through COBS stream

    //qDebug() << part;
    _request_format_time.start();
    _req.request(mandala::cmd::env::telemetry::format::uid);
    xbus::telemetry::format_req_s r{xbus::telemetry::fmt_version, part};
    r.write(&_req);
    trace()->block(QString::number(part));
    _req.send();
}

bool PApxTelemetry::unpack_xpdr(PStreamReader &stream)
{
    if (stream.available() != xbus::telemetry::xpdr_s::psize())
        return false;

    xbus::telemetry::xpdr_s xpdr;
    xpdr.read(&stream);

    if (xpdr.version != xbus::telemetry::xpdr_s::current_version) {
        qDebug() << "XPDR format error" << xpdr.version;
        return true;
    }

    PBase::Values values;

    values.insert(mandala::est::nav::pos::lat::uid, xpdr.lat);
    values.insert(mandala::est::nav::pos::lon::uid, xpdr.lon);
    values.insert(mandala::est::nav::pos::hmsl::uid, xpdr.hmsl);

    values.insert(mandala::est::nav::pos::speed::uid, xpdr.speed);
    values.insert(mandala::est::nav::pos::vspeed::uid, xpdr.vspeed);
    values.insert(mandala::est::nav::pos::bearing::uid, xpdr.bearing);
    values.insert(mandala::cmd::nav::proc::mode::uid, xpdr.mode);

    emit xpdrData(values);
    return true;
}

void PApxTelemetry::requestTelemetry()
{
    _req.request(mandala::cmd::env::telemetry::data::uid);
    _req.send();
}
