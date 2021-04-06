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
#include "PApx.h"

#include "PApxFirmware.h"

#include <App/App.h>
#include <Mandala/Mandala.h>

PApx::PApx(Fact *parent)
    : PBase(parent, "papx", tr("APX Protocol"), tr("Standard data format for APX Autopilot"))
    , _req(this)
{
    // delayed requests timer
    _reqTimer.setInterval(500);
    connect(&_reqTimer, &QTimer::timeout, this, &PApx::request_next);

    connect(App::instance(), &App::loadingFinished, this, &PApx::updateLocal, Qt::QueuedConnection);
}

void PApx::updateLocal()
{
    m_local = new PApxVehicle(this, "LOCAL", "", PVehicle::UAV, 0);
    m_firmware = new PApxFirmware(this);
    emit vehicle_available(m_local);
}

void PApx::process_downlink(QByteArray packet)
{
    if (!m_local) // not loaded yet
        return;

    if (static_cast<size_t>(packet.size()) > xbus::size_packet_max)
        return;

    PStreamReader stream(packet);

    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "packet" << packet.toHex().toUpper();
        return;
    }
    xbus::pid_s pid;
    pid.read(&stream);

    switch (pid.uid) {
    default:
        break;

    case mandala::cmd::env::vehicle::ident::uid: {
        findParent<PApx>()->trace_pid(pid);
        if (pid.pri == xbus::pri_request)
            return;

        if (stream.available() <= sizeof(xbus::vehicle::squawk_t))
            return;
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        auto squawkText = PApx::squawkText(squawk);

        trace()->block(squawkText);

        if (stream.available() <= xbus::vehicle::ident_s::psize())
            return;

        trace()->data(stream.payload());

        xbus::vehicle::ident_s ident;
        ident.read(&stream);

        const char *s = stream.read_string(stream.available());
        if (!s || stream.available() > 0)
            return;

        auto callsign = QString(s).trimmed();

        if ((!squawk) || callsign.isEmpty()) {
            //received zero SQUAWK
            assign_squawk(ident, callsign);
            return;
        }

        QString uid = QByteArray(reinterpret_cast<const char *>(ident.uid),
                                 sizeof(xbus::vehicle::uid_t))
                          .toHex()
                          .toUpper();

        PVehicle::VehicleType type = ident.flags.gcs ? PVehicle::GCS : PVehicle::UAV;

        // lookup vehicle by uid
        PApxVehicle *available = nullptr;
        for (auto c : facts()) {
            PApxVehicle *i = static_cast<PApxVehicle *>(c);
            if (i->uid() == uid) {
                available = i;
                break;
            }
        }

        // lookup vehicle by squawk
        PApxVehicle *identified = _squawk_map.value(squawk);

        if (available) {
            available->packetReceived(pid.uid);
            if (!identified) {
                _squawk_map.insert(squawk, available);
                qDebug() << "re-identified" << available->title();
            } else if (available != identified) {
                qWarning() << "duplicate squawk: " << squawkText;
                _squawk_map.remove(squawk);
                _squawk_map.remove(_squawk_map.key(available));
                _squawk_map.remove(_squawk_map.key(identified));
                assign_squawk(ident, callsign);
                return;
            }
            // update matched by uid info from ident
            available->setTitle(callsign);
            available->setVehicleType(type);
            available->setSquawk(squawk);
        } else {
            // vehicle not created
            if (identified) {
                qWarning() << "change squawk: " << PApx::squawkText(squawk);
                _squawk_map.remove(squawk);
                assign_squawk(ident, callsign);
                return;
            } else {
                // add new vehicle
                available = new PApxVehicle(this, callsign, uid, type, squawk);
                _squawk_map.insert(squawk, available);
                connect(available, &Fact::removed, this, [this, available]() {
                    _squawk_map.remove(_squawk_map.key(available));
                });
                emit vehicle_available(available);
                available->packetReceived(pid.uid);
            }
        }
        return;
    }

    case mandala::cmd::env::vehicle::downlink::uid: {
        findParent<PApx>()->trace_pid(pid);
        if (pid.pri == xbus::pri_request)
            return;

        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (stream.available() == 0)
            return;
        trace()->block(PApx::squawkText(squawk));

        if (!squawk)
            return; //broadcast?
        //check if new transponder detected, request IDENT
        auto *v = _squawk_map.value(squawk);
        if (v) {
            v->packetReceived(pid.uid);
            trace()->block(v->title().append(':'));
            trace()->tree();
            v->process_downlink(stream);
        } else {
            trace()->data(stream.payload());
            request_ident_schedule(squawk);
        }
        return;
    }
    }

    // is not vehicle wrapped format - forward to local
    stream.reset();
    m_local->process_downlink(stream);
}

void PApx::request_ident_schedule(xbus::vehicle::squawk_t squawk)
{
    if (_req_ident.contains(squawk))
        return;

    _req_ident.append(squawk);
    if (!_reqTimer.isActive())
        request_next();
    _reqTimer.start();
}
void PApx::request_next()
{
    if (!_req_ident.isEmpty()) {
        request_ident(_req_ident.takeFirst());
        return;
    }
    _reqTimer.stop();
}

void PApx::assign_squawk(const xbus::vehicle::ident_s &ident, QString callsign)
{
    qDebug() << callsign;

    //generate squawk
    xbus::vehicle::squawk_t squawk = 0xA5AD;
    xbus::vehicle::squawk_t tcnt = 32767;
    do {
        squawk = (static_cast<xbus::vehicle::squawk_t>(QRandomGenerator::global()->generate())
                  + tcnt)
                 ^ squawk;
        if (squawk < 100 || squawk > 0xff00)
            continue;
        if (_squawk_map.contains(squawk))
            continue;
        if (_squawk_blacklist.contains(squawk))
            continue;
        break;
    } while (tcnt--);
    if (!tcnt) {
        qDebug() << "Can't find new squawk for assignment";
        return;
    }
    _squawk_blacklist.append(squawk);
    if (_squawk_blacklist.size() > 1000)
        _squawk_blacklist.takeFirst();

    _req.request(mandala::cmd::env::vehicle::ident::uid);
    _req.write<xbus::vehicle::squawk_t>(squawk);
    trace()->block(PApx::squawkText(squawk));

    ident.write(&_req);
    trace()->raw(ident, "ident");

    //unique squawk assigned, update callsign
    QString s = callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(PApx::squawkText(squawk));
    s = s.toUpper();

    _req.write_string(s.toUtf8());
    trace()->block(s);

    _req.send();
}
void PApx::request_ident(xbus::vehicle::squawk_t squawk)
{
    _req.request(mandala::cmd::env::vehicle::ident::uid);
    _req.write<xbus::vehicle::squawk_t>(squawk);
    trace()->block(PApx::squawkText(squawk));
    _req.send();
}

void PApx::trace_pid(const xbus::pid_s &pid)
{
    if (!trace()->enabled())
        return;

    trace_uid(pid.uid);

    QString s;
    switch (pid.pri) {
    case xbus::pri_final:
        s = "F";
        break;
    case xbus::pri_primary:
        s = "P";
        break;
    case xbus::pri_secondary:
        s = "S";
        break;
    case xbus::pri_failsafe:
        s = "E";
        break;
    case xbus::pri_response:
        s = "R";
        break;
    case xbus::pri_request:
        s = "Q";
        break;
    default:
        s = QString::number(static_cast<int>(pid.pri));
    }
    s = QString("%1%2").arg(s).arg(QString::number(static_cast<int>(pid.seq)));
    trace()->block(s);
}
void PApx::trace_uid(mandala::uid_t uid)
{
    if (!trace()->enabled())
        return;
    trace()->block(QString("$%1").arg(Mandala::meta(uid).path));
}
