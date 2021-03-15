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

#include <Mandala/Mandala.h>

PApx::PApx(QObject *parent)
    : PBase("apx", parent)
{}

void PApx::process_downlink(QByteArray packet)
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

    trace(pid);

    switch (pid.uid) {
    default:
        stream.reset();
        //local->downlink(stream);
        return;

    case mandala::cmd::env::vehicle::ident::uid: {
        if (pid.pri == xbus::pri_request)
            return;

        if (stream.available() <= sizeof(xbus::vehicle::squawk_t))
            break;
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        trace_block(QString::number(squawk, 16));

        if (stream.available() <= xbus::vehicle::ident_s::psize())
            break;

        trace_data(stream.payload());

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

        //find uav in list by uid
        /*PApxVehicle *v = nullptr;
        for (auto i : _vehicles) {
            if (i->match(ident.uid)) {
                v = i;
                break;
            }
        }
        // check duplicate squawk
        if (v && squawkMap.value(squawk) != v) {
            identAssign(ident, callsign);
            qWarning() << "duplicate squawk: " << QString::number(squawk, 16);
            break;
        }

        if (v) {
            qDebug() << "vehicle protocol exists";
            //update from ident
            if (!(v->match(squawk) && v->match(ident) && v->title() == callsign)) {
                squawkMap.remove(v->squawk());
                squawkMap.remove(squawkMap.key(v));
                squawkMap.insert(squawk, v);
                v->updateIdent(squawk, ident, callsign);
                qDebug() << "vehicle ident updated";
            }
        } else {
            //new Vehicle found
            v = addVehicle(squawk, ident, callsign);
            squawkMap.insert(squawk, v);
            emit vehicleIdentified(v);
        }
        v->receivedCmdEnvPacket(pid.uid);
        */
    } break;
    case mandala::cmd::env::vehicle::downlink::uid: {
        if (pid.pri == xbus::pri_request)
            return;

        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (stream.available() == 0)
            break;
        //trace_downlink(ProtocolTraceItem::SQUAWK, QString::number(squawk, 16));

        if (!squawk)
            break; //broadcast?
        //check if new transponder detected, request IDENT
        /* ProtocolVehicle *v = squawkMap.value(squawk);
        if (v) {
            v->downlink(stream);
            v->receivedCmdEnvPacket(pid.uid);
            break;
        }
        trace_downlink(stream.payload());
        identRequest(squawk);*/
    } break;
    }
}

void PApx::trace(const xbus::pid_s &pid)
{
    if (!_trace_enabled)
        return;

    trace_block(Mandala::meta(pid.uid).name);

    QString s;
    switch (pid.pri) {
    case xbus::pri_primary:
        s = "pri ";
        break;
    case xbus::pri_secondary:
        s = "sec ";
        break;
    case xbus::pri_failsafe:
        s = "fsf ";
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
    trace_block(s);
}

void PApx::assign_squawk(const xbus::vehicle::ident_s &ident, const QString &callsign)
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
        if (_vehicles.contains(squawk))
            continue;
        break;
    } while (tcnt--);
    if (!tcnt) {
        qDebug() << "Can't find new squawk for assignment";
        return;
    }

    _ostream.req(mandala::cmd::env::vehicle::ident::uid);
    _ostream.write<xbus::vehicle::squawk_t>(squawk);

    //unique squawk assigned, update callsign
    QString s = callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(static_cast<ulong>(squawk), 4, 16, QLatin1Char('0')).toUpper();
    s = s.toUpper();
    ident.write(&_ostream);

    send_uplink(_ostream.toByteArray());
}
