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
#include "PApxData.h"

PApxData::PApxData(PApxVehicle *parent)
    : PData(parent)
    , _vehicle(parent)
    , _req(parent)
{}

bool PApxData::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    switch (pid.uid) {
    default:
        return false;
    case mandala::cmd::env::stream::vcp::uid:
        if (stream.available() > 1) {
            uint8_t port_id = stream.read<uint8_t>();
            trace()->block(QString::number(port_id));
            trace()->data(stream.payload());
            emit serialData(port_id, stream.payload());
            return true;
        }
        qWarning() << "Empty serial data received";
        break;
    case mandala::cmd::env::script::jsexec::uid:
        if (stream.available() > 2) {
            QString script = stream.payload().trimmed();
            trace()->block(script);
            if (!script.isEmpty()) {
                emit jsexecData(script);
                return true;
            }
        }
        qWarning() << "Empty jsexec data received" << stream.dump_payload();
        break;
    case mandala::cmd::env::stream::calib::uid:
        if (stream.available() > sizeof(mandala::uid_t)) {
            mandala::uid_t uid;
            stream >> uid;
            findParent<PApx>()->trace_uid(uid);
            trace()->data(stream.payload());
            emit calibrationData(uid, stream.payload());
            return true;
        }
        qWarning() << "Empty calibration data received";
        break;
    }
    _vehicle->incErrcnt();
    return false;
}

void PApxData::requestCalibration(mandala::uid_t uid, QByteArray data)
{
    _req.request(mandala::cmd::env::stream::calib::uid);
    _req << uid;
    findParent<PApx>()->trace_uid(uid);
    _req.append(data);
    trace()->data(data);
    _req.send();
}

void PApxData::requestScript(QString func)
{
    func = func.simplified().trimmed();
    if (func.isEmpty())
        return;
    _req.request(mandala::cmd::env::script::vmexec::uid);
    _req.append(func.toUtf8());
    trace()->block(func);
    _req.send();
}

void PApxData::sendSerial(quint8 portID, QByteArray data)
{
    _req.request(mandala::cmd::env::stream::vcp::uid);
    _req.write<uint8_t>(portID);
    trace()->block(QString::number(portID));
    _req.append(data);
    trace()->data(data);
    _req.send();
}

void PApxData::flyTo(qreal lat, qreal lon)
{
    mandala::bundle::cmd_pos_s data{};
    data.lat = lat;
    data.lon = lon;
    sendBundle<mandala::bundle::cmd_pos_s>(mandala::cmd::nav::pos::uid, data);
}

void PApxData::pack(const QVariant &v, mandala::type_id_e type, PStreamWriter &stream)
{
    switch (type) {
    default:
        return;
    case mandala::type_real:
        stream.write<mandala::real_t>(v.value<mandala::real_t>());
        return;
    case mandala::type_dword:
        stream.write<mandala::dword_t>(v.value<mandala::dword_t>());
        return;
    case mandala::type_word:
        stream.write<mandala::word_t>(v.value<mandala::word_t>());
        return;
    case mandala::type_byte:
        stream.write<mandala::byte_t>(v.value<mandala::byte_t>());
        return;
    case mandala::type_option:
        stream.write<mandala::option_t>(v.value<mandala::option_t>());
        return;
    }
}

void PApxData::sendValue(mandala::uid_t uid, QVariant value)
{
    _req.request(uid, value.isNull() ? xbus::pri_request : xbus::pri_final);
    mandala::spec_s spec{};
    spec.type = Mandala::meta(uid).type_id;
    spec.write(&_req);
    trace()->raw(spec);

    if (!value.isNull()) {
        size_t spos = _req.pos();
        pack(value, spec.type, _req);
        trace()->data(_req.toByteArray(spos));
    }
    _req.send();
}
