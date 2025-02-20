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

#include <Fleet/Fleet.h>
#include <Mandala/Mandala.h>

static QStringList path_pos_ll{"cmd.pos", "est.pos", "est.ref"};

PApxData::PApxData(PApxUnit *parent)
    : PData(parent)
    , _req(parent)
{}

bool PApxData::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    bool is_request = pid.pri == xbus::pri_request;
    const auto uid = pid.uid;
    do {
        if (uid < xbus::cmd::uid) {
            // data value received
            if (is_request)
                return true;

            // check for bundles sent from other GCS
            const auto path = Mandala::mpath(uid);
            if (mandala::is_bundle(uid) && path_pos_ll.contains(path)) {
                mandala::bundle::pos_ll_s d;
                stream.read(&d, sizeof(d));

                auto m = Fleet::instance()->current()->f_mandala;
                auto lat = qobject_cast<MandalaFact *>(m->fact(path + ".lat"));
                auto lon = qobject_cast<MandalaFact *>(m->fact(path + ".lon"));
                if (lat && lon) {
                    lat->setRawValueLocal(mandala::a32_to_deg(d.lat));
                    lon->setRawValueLocal(mandala::a32_to_deg(d.lon));
                }
                return true;
            }

            // unpack data value
            if (stream.available() <= mandala::spec_s::psize()) {
                qWarning() << "size" << stream.available();
                break;
            }

            mandala::spec_s spec;
            spec.read(&stream);
            trace()->block(QString("T%1").arg(spec.type));

            trace()->data(stream.payload());

            PBase::Values values = unpack(pid, spec, stream);
            if (values.isEmpty() || stream.available() > 0) {
                qWarning() << "unpack data values error";
                break;
            }
            emit valuesData(values);
            return true;
        }

        switch (uid) {
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
            if (is_request)
                return true;
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
    } while (0);

    //error
    trace()->block("ERR:");
    trace()->data(stream.payload());

    _unit->incErrcnt();
    return true;
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

void PApxData::pack(const QVariant &v, mandala::type_id_e type, PStreamWriter &stream)
{
    switch (type) {
    case mandala::type_byte:
        stream.write<mandala::byte_t>(v.value<mandala::byte_t>());
        return;
    case mandala::type_word:
        stream.write<mandala::word_t>(v.value<mandala::word_t>());
        return;
    case mandala::type_dword:
        stream.write<mandala::dword_t>(v.value<mandala::dword_t>());
        return;
    case mandala::type_real:
        stream.write<mandala::real_t>(v.value<mandala::real_t>());
        return;
    }
}

void PApxData::sendValue(mandala::uid_t uid, QVariant value)
{
    if (mandala::is_bundle(uid)) {
        sendBundle(uid, value);
        return;
    }

    _req.request(uid);
    mandala::spec_s spec{};
    spec.type = Mandala::meta(uid).type_id;
    spec.write(&_req);
    trace()->block(QString("T%1").arg(spec.type));

    if (!value.isNull()) {
        size_t spos = _req.pos();
        pack(value, spec.type, _req);
        trace()->data(_req.toByteArray(spos));
    }
    _req.send();
}

PBase::Values PApxData::unpack(const xbus::pid_s &pid,
                               const mandala::spec_s &spec,
                               PStreamReader &stream)
{
    PBase::Values values;

    if (pid.pri > 0) {
        qWarning() << "pri:" << pid.pri << Mandala::meta(pid.uid).path;
    }

    //qDebug() << Mandala::meta(pid.uid).name << stream->dump_payload();
    QVariant v;
    switch (spec.type) {
    case mandala::type_byte:
        v = unpack<mandala::byte_t>(stream);
        break;
    case mandala::type_word:
        v = unpack<mandala::word_t>(stream);
        break;
    case mandala::type_dword:
        v = unpack<mandala::dword_t>(stream);
        break;
    case mandala::type_real:
        v = unpack<mandala::real_t>(stream);
        break;
    }
    values.push_back({pid.uid, v});

    if (v.isNull()) {
        values.clear();
        // error
        qDebug() << "error: " << Mandala::meta(pid.uid).path << stream.available()
                 << stream.dump_payload();
    }

    return values;
}

void PApxData::sendBundle(mandala::uid_t uid, QVariant value)
{
    const auto path = Mandala::mpath(uid);
    if (path_pos_ll.contains(path)) {
        QVariantList v = value.value<QVariantList>();
        if (v.size() != 2)
            return;
        sendBundleT<mandala::bundle::pos_ll_s>(uid,
                                               {mandala::deg_to_a32(v.at(0).toDouble()),
                                                mandala::deg_to_a32(v.at(1).toDouble())});
        return;
    }

    if (path == "cmd.nav.ats") {
        QVariantList v = value.value<QVariantList>();
        if (v.size() != 3)
            return;
        sendBundleT<mandala::bundle::pos_llh_s>(uid,
                                                {mandala::deg_to_a32(v.at(0).toDouble()),
                                                 mandala::deg_to_a32(v.at(1).toDouble()),
                                                 v.at(2).toFloat()});
        return;
    }

    _nimp(__FUNCTION__);
}
