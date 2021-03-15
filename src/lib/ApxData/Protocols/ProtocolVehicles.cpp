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
#include "ProtocolVehicles.h"
#include "ProtocolVehicle.h"

#include <xbus/XbusPacket.h>
#include <xbus/XbusVehicle.h>

ProtocolVehicles::ProtocolVehicles(QObject *parent)
    : ProtocolBase(parent, "protocols")
{
    setTitle(tr("Protocols"));
    setDescr(tr("Data exchange interfaces"));
    setIcon("contain");
    setDataType(Count);

    m_trace = new ProtocolTrace(this);

    //create base vehicles
    xbus::vehicle::ident_s ident;
    memset(&ident, 0, sizeof(ident));
    local = new ProtocolVehicle(this, 0, ident, "LOCAL", ProtocolVehicle::LOCAL);
    ident.flags.gcs = 1;
    replay = new ProtocolVehicle(this, 0, ident, "REPLAY", ProtocolVehicle::REPLY);

    // delayed requests timer
    reqTimer.setInterval(500);
    connect(&reqTimer, &QTimer::timeout, this, [this]() {
        if (reqList.isEmpty())
            reqTimer.stop();
        else
            process_uplink(reqList.takeFirst());
    });
}

void ProtocolVehicles::downlink(const QByteArray packet)
{
    process_downlink(packet);
}
void ProtocolVehicles::process_uplink(const QByteArray packet)
{
    trace_uplink_packet(packet);
    emit uplink(packet);
    //qDebug() << packet.toHex().toUpper();
}

void ProtocolVehicles::process_downlink(const QByteArray packet)
{
    if (static_cast<size_t>(packet.size()) > xbus::size_packet_max)
        return;

    //qDebug() << "rx" << packet.toHex().toUpper();

    ProtocolStreamReader stream(packet);

    trace_downlink(ProtocolTraceItem::PACKET, QString::number(stream.available()));

    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "packet" << packet.toHex().toUpper();
        return;
    }
    xbus::pid_s pid;
    pid.read(&stream);

    trace_downlink(pid);
    //qDebug() << "rx" << QString::number(pid, 16) << packet.toHex().toUpper();

    switch (pid.uid) {
    default:
        stream.reset();
        local->downlink(stream);
        return;

        /*case mandala::cmd::env::vehicle::xpdr::uid: { //transponder from UAV received
        if (pid.pri == xbus::pri_request)
            return;

        if (stream.available() <= sizeof(xbus::vehicle::squawk_t))
            break;
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();

        if (stream.available() != xbus::vehicle::xpdr_s::psize())
            break;

        trace_downlink(stream.payload());

        ProtocolVehicle *v = squawkMap.value(squawk);
        if (!v) {
            //new transponder detected, request IDENT
            identRequest(squawk);
            break;
        }

        xbus::vehicle::xpdr_s xpdr;
        xpdr.read(&stream);

        v->xpdrData(xpdr);
    } break;*/

    case mandala::cmd::env::vehicle::ident::uid: {
        if (pid.pri == xbus::pri_request)
            return;

        if (stream.available() <= sizeof(xbus::vehicle::squawk_t))
            break;
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        trace_downlink(ProtocolTraceItem::SQUAWK, QString::number(squawk, 16));

        if (stream.available() <= xbus::vehicle::ident_s::psize())
            break;

        trace_downlink(stream.payload());

        xbus::vehicle::ident_s ident;
        ident.read(&stream);

        const char *s = stream.read_string(stream.available());
        if (!s || stream.available() > 0)
            break;

        QString callsign = QString(s).trimmed();

        if ((!squawk) || callsign.isEmpty()) {
            //received zero SQUAWK
            identAssign(ident, callsign);
            break;
        }

        //find uav in list by uid
        ProtocolVehicle *v = nullptr;
        for (auto i : squawkMap) {
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
    } break;
    case mandala::cmd::env::vehicle::downlink::uid: {
        if (pid.pri == xbus::pri_request)
            return;

        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (stream.available() == 0)
            break;
        trace_downlink(ProtocolTraceItem::SQUAWK, QString::number(squawk, 16));

        if (!squawk)
            break; //broadcast?
        //check if new transponder detected, request IDENT
        ProtocolVehicle *v = squawkMap.value(squawk);
        if (v) {
            v->downlink(stream);
            v->receivedCmdEnvPacket(pid.uid);
            break;
        }
        trace_downlink(stream.payload());
        identRequest(squawk);
    } break;
    }
}

ProtocolVehicle *ProtocolVehicles::addVehicle(xbus::vehicle::squawk_t squawk,
                                              const xbus::vehicle::ident_s &ident,
                                              const QString &callsign)
{
    qDebug() << "new vehicle protocol";
    ProtocolVehicle *v = new ProtocolVehicle(this,
                                             squawk,
                                             ident,
                                             callsign,
                                             ProtocolVehicle::IDENTIFIED);
    return v;
}

void ProtocolVehicles::identRequest(xbus::vehicle::squawk_t squawk)
{
    //qDebug() << "scheduled ident req";
    ostream.req(mandala::cmd::env::vehicle::ident::uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);

    // schedule
    if (!reqList.contains(ostream.toByteArray())) {
        reqList.append(ostream.toByteArray());
        reqTimer.start();
    }
}

void ProtocolVehicles::identAssign(const xbus::vehicle::ident_s &ident, const QString &callsign)
{
    qDebug() << "assign" << callsign;

    //generate squawk
    xbus::vehicle::squawk_t squawk = 0xA5AD;
    xbus::vehicle::squawk_t tcnt = 32767;
    do {
        squawk = (static_cast<xbus::vehicle::squawk_t>(QRandomGenerator::global()->generate())
                  + tcnt)
                 ^ squawk;
        if (squawk < 100 || squawk > 0xff00)
            continue;
        if (squawkMap.contains(squawk))
            continue;
        break;
    } while (tcnt--);
    if (!tcnt) {
        qDebug() << "Can't find new squawk for assignment";
        return;
    }

    ostream.req(mandala::cmd::env::vehicle::ident::uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);

    //unique squawk assigned, update callsign
    QString s = callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(static_cast<ulong>(squawk), 4, 16, QLatin1Char('0')).toUpper();
    s = s.toUpper();
    ident.write(&ostream);

    process_uplink(ostream.toByteArray());
}

void ProtocolVehicles::send(xbus::vehicle::squawk_t squawk, QByteArray packet)
{
    //qDebug() << payload.toHex();
    if (!squawk) {
        //local vehicle
        process_uplink(packet);
        return;
    }

    ostream.req(mandala::cmd::env::vehicle::uplink::uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);
    ostream.append(packet);

    process_uplink(ostream.toByteArray());
}

void ProtocolVehicles::sendHeartbeat()
{
    ostream.req(mandala::cmd::env::vehicle::uplink::uid);
    process_uplink(ostream.toByteArray());
}
