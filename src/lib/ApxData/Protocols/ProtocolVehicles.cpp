/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "ProtocolVehicles.h"
#include "ProtocolConverter.h"
#include "ProtocolServiceFirmware.h"
#include "ProtocolVehicle.h"

#include <Mandala/Mandala.h>

#include <Xbus/XbusPacket.h>
#include <Xbus/XbusVehicle.h>

ProtocolVehicles::ProtocolVehicles(QObject *parent)
    : ProtocolBase(parent)
{
    xbus::vehicle::ident_s ident;
    memset(&ident, 0, sizeof(ident));
    ident.vclass = 0;
    local = new ProtocolVehicle(0, ident, this);
    //firmware = new ProtocolServiceFirmware(local->service);

    reqTimer.setInterval(500);
    connect(&reqTimer, &QTimer::timeout, this, [this]() {
        if (reqList.isEmpty())
            reqTimer.stop();
        else
            process_uplink(reqList.takeFirst());
    });
}

void ProtocolVehicles::setConverter(ProtocolConverter *c)
{
    if (m_converter) {
        disconnect(m_converter, nullptr, this, nullptr);
    }
    m_converter = c;
    if (m_converter) {
        connect(m_converter, &ProtocolConverter::uplink, this, &ProtocolVehicles::uplink);
        connect(m_converter,
                &ProtocolConverter::downlink,
                this,
                &ProtocolVehicles::process_downlink);
    }
}
ProtocolConverter *ProtocolVehicles::converter() const
{
    return m_converter;
}

void ProtocolVehicles::downlink(const QByteArray packet)
{
    if (m_converter)
        m_converter->convertDownlink(packet);
    else
        process_downlink(packet);
}
void ProtocolVehicles::process_uplink(const QByteArray packet)
{
    if (m_converter)
        m_converter->convertUplink(packet);
    else
        emit uplink(packet);
    //qDebug() << packet.toHex().toUpper();
}

void ProtocolVehicles::process_downlink(const QByteArray packet)
{
    if (static_cast<size_t>(packet.size()) > xbus::size_packet_max)
        return;

    //qDebug() << "rx" << packet.toHex().toUpper();

    ProtocolStreamReader stream(packet);
    if (stream.available() < sizeof(xbus::pid_t)) {
        qWarning() << "packet" << packet.toHex().toUpper();
        return;
    }

    xbus::pid_t pid = stream.read<xbus::pid_t>();
    //qDebug() << "rx" << QString::number(pid, 16) << packet.toHex().toUpper();

    switch (pid) {
    default:
        stream.reset();
        local->downlink(stream);
        break;
    case mandala::cmd::env::vehicle::xpdr::uid: { //transponder from UAV received
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (xbus::vehicle::xpdr_s::psize() != stream.available())
            break;

        ProtocolVehicle *v = squawkMap.value(squawk);
        if (!v) {
            //new transponder detected, request IDENT
            identRequest(squawk);
            break;
        }

        xbus::vehicle::xpdr_s xpdr;
        xpdr.read(&stream);

        v->xpdrData(xpdr);
    } break;
    case mandala::cmd::env::vehicle::ident::uid: {
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        qDebug() << "ident received" << squawk;
        if (xbus::vehicle::ident_s::psize() != stream.available())
            break;

        xbus::vehicle::ident_s ident;
        ident.read(&stream);

        if ((!squawk) || ident.callsign[0] == 0) {
            //received zero SQUAWK
            identAssign(squawk, ident);
            break;
        }
        //find uav in list by uid
        ProtocolVehicle *v = nullptr;
        for (auto i : squawkMap) {
            if (i->ident.vuid == ident.vuid) {
                v = i;
                break;
            }
        }
        if (v) {
            qDebug() << "vehicle protocol exists";
            //update from ident
            if (v->squawk != squawk || memcmp(&(v->ident), &ident, sizeof(ident)) != 0) {
                squawkMap.remove(v->squawk);
                squawkMap.remove(squawkMap.key(v));
                squawkMap.insert(squawk, v);
                v->squawk = squawk;
                v->ident = ident;
                qDebug() << "vehicle ident updated";
                v->identUpdated();
            }
        } else {
            //new Vehicle found
            v = addVehicle(squawk, ident);
            emit vehicleIdentified(v);
        }
        //check squawk with uid
        if (squawkMap.contains(squawk)) {
            if (squawkMap.value(squawk) != v && ident.vuid != v->ident.vuid) {
                //duplicate squawk came with this ident
                emit identAssigned(v, ident);
                identAssign(squawk, ident);
                break;
            }
        } else {
            squawkMap.insert(squawk, v);
        }
    } break;
    case mandala::cmd::env::vehicle::downlink::uid: {
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (stream.available() == 0)
            break;

        if (!squawk)
            break; //broadcast?
        //check if new transponder detected, request IDENT
        ProtocolVehicle *v = squawkMap.value(squawk);
        if (v)
            v->downlink(stream);
        else
            identRequest(squawk);
    } break;
    }
}

ProtocolVehicle *ProtocolVehicles::addVehicle(xbus::vehicle::squawk_t squawk,
                                              const xbus::vehicle::ident_s &ident)
{
    qDebug() << "new vehicle protocol";
    ProtocolVehicle *v = new ProtocolVehicle(squawk, ident, this);
    return v;
}

void ProtocolVehicles::identRequest(xbus::vehicle::squawk_t squawk)
{
    //qDebug() << "scheduled ident req";
    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::ident::uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);
    // schedule
    if (!reqList.contains(ostream.toByteArray())) {
        reqList.append(ostream.toByteArray());
        reqTimer.start();
    }
}

void ProtocolVehicles::identAssign(xbus::vehicle::squawk_t squawk,
                                   const xbus::vehicle::ident_s &ident)
{
    qDebug() << "assign" << squawk << ident.callsign << ident.vclass;

    //generate squawk
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

    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::ident::uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);

    xbus::vehicle::ident_s dIdent;

    //unique squawk assigned, update callsign
    QString s = ident.callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(static_cast<ulong>(squawk), 4, 16, QLatin1Char('0')).toUpper();
    s.truncate(sizeof(dIdent.callsign) - 1);
    s = s.toUpper();
    memset(dIdent.callsign, 0, sizeof(dIdent.callsign));
    memcpy(dIdent.callsign, s.toUtf8().data(), static_cast<uint>(s.size()));

    memcpy(dIdent.vuid, ident.vuid, sizeof(ident.vuid));

    dIdent.vclass = ident.vclass;
    dIdent.write(&ostream);

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
    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::uplink::uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);
    ostream.append(packet);
    process_uplink(ostream.toByteArray());
}

void ProtocolVehicles::sendHeartbeat()
{
    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::uplink::uid);
    process_uplink(ostream.toByteArray());
}
