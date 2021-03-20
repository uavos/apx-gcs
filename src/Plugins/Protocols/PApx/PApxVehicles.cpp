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
#include "PApxVehicles.h"
#include "PApx.h"

PApxVehicles::PApxVehicles(PApx *parent)
    : PVehicles(parent)
    , _papx(parent)
    , _req(parent)
{
    m_local = new PApxVehicle(this, "LOCAL", "", PVehicle::UAV, 0);

    // delayed requests timer
    _reqTimer.setInterval(500);
    connect(&_reqTimer, &QTimer::timeout, this, &PApxVehicles::request_next);
}

void PApxVehicles::process_downlink(QByteArray packet)
{
    if (static_cast<size_t>(packet.size()) > xbus::size_packet_max)
        return;

    PStreamReader stream(packet);

    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "packet" << packet.toHex().toUpper();
        return;
    }
    xbus::pid_s pid;
    pid.read(&stream);

    parent<PApx>()->trace_pid(pid);

    switch (pid.uid) {
    default:
        m_local->process_downlink(packet);
        return;

    case mandala::cmd::env::vehicle::ident::uid: {
        if (pid.pri == xbus::pri_request)
            return;

        if (stream.available() <= sizeof(xbus::vehicle::squawk_t))
            break;
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        QString squawkText = PApx::squawkText(squawk);

        trace()->block(squawkText);

        if (stream.available() <= xbus::vehicle::ident_s::psize())
            break;

        trace()->data(stream.payload());

        xbus::vehicle::ident_s ident;
        ident.read(&stream);

        const char *s = stream.read_string(stream.available());
        if (!s || stream.available() > 0)
            break;

        QString callsign = QString(s).trimmed();

        if ((!squawk) || callsign.isEmpty()) {
            //received zero SQUAWK
            assign_squawk(ident, callsign);
            break;
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
            if (!identified) {
                _squawk_map.insert(squawk, available);
                qDebug() << "re-identified" << available->title();
            } else if (available != identified) {
                qWarning() << "duplicate squawk: " << squawkText;
                _squawk_map.remove(squawk);
                _squawk_map.remove(_squawk_map.key(available));
                _squawk_map.remove(_squawk_map.key(identified));
                assign_squawk(ident, callsign);
                break;
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
                break;
            } else {
                // add new vehicle
                available = new PApxVehicle(this, callsign, uid, type, squawk);
                _squawk_map.insert(squawk, available);
                connect(available, &Fact::removed, this, [this, available]() {
                    _squawk_map.remove(_squawk_map.key(available));
                });
                emit vehicle_available(available);
            }
        }
    } break;
    case mandala::cmd::env::vehicle::downlink::uid: {
        if (pid.pri == xbus::pri_request)
            return;

        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (stream.available() == 0)
            break;
        trace()->block(PApx::squawkText(squawk));

        if (!squawk)
            break; //broadcast?
        //check if new transponder detected, request IDENT
        PApxVehicle *v = _squawk_map.value(squawk);
        if (v) {
            trace()->tree();
            trace()->block(v->title().append(':'));
            v->process_downlink(stream.payload());
        } else {
            trace()->data(stream.payload());
            request_ident_schedule(squawk);
        }
    } break;
    }
}

void PApxVehicles::request_ident_schedule(xbus::vehicle::squawk_t squawk)
{
    if (_req_ident.contains(squawk))
        return;

    _req_ident.append(squawk);
    if (!_reqTimer.isActive())
        request_next();
    _reqTimer.start();
}
void PApxVehicles::request_next()
{
    if (!_req_ident.isEmpty()) {
        request_ident(_req_ident.takeFirst());
        return;
    }
    _reqTimer.stop();
}

void PApxVehicles::assign_squawk(const xbus::vehicle::ident_s &ident, QString callsign)
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
    trace()->raw("ident", ident);

    //unique squawk assigned, update callsign
    QString s = callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(PApx::squawkText(squawk));
    s = s.toUpper();

    _req.write_string(s.toUtf8());
    trace()->block(s);

    _req.send();
}
void PApxVehicles::request_ident(xbus::vehicle::squawk_t squawk)
{
    _req.request(mandala::cmd::env::vehicle::ident::uid);
    _req.write<xbus::vehicle::squawk_t>(squawk);
    trace()->block(PApx::squawkText(squawk));
    _req.send();
}
