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
    IdentData ident;
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

    switch (pid) {
    default:
        stream.reset();
        local->downlink(stream);
        break;
    case mandala::cmd::env::vehicle::xpdr::meta.uid: { //transponder from UAV received
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

        XpdrData d;
        d.lat = static_cast<double>(xpdr.lat);
        d.lon = static_cast<double>(xpdr.lon);
        d.alt = static_cast<double>(xpdr.alt);
        d.gSpeed = static_cast<double>(xpdr.speed);
        d.course = static_cast<double>(xpdr.course);
        d.mode = xpdr.mode;
        v->xpdrData(d);
    } break;
    case mandala::cmd::env::vehicle::ident::meta.uid: {
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        qDebug() << "ident received" << squawk;
        if (xbus::vehicle::ident_s::psize() != stream.available())
            break;

        xbus::vehicle::ident_s ident;
        ident.read(&stream);

        IdentData d;
        d.callsign = QString(QByteArray(ident.callsign, sizeof(ident.callsign))).trimmed();
        d.uid = QByteArray(reinterpret_cast<const char *>(ident.vuid), sizeof(ident.vuid))
                    .toHex()
                    .toUpper();
        d.vclass = ident.vclass;
        //emit identData(ident->squawk,d);
        if ((!squawk) || d.callsign.isEmpty()) {
            //received zero SQUAWK
            identAssign(squawk, d);
            break;
        }
        //find uav in list by uid
        ProtocolVehicle *v = nullptr;
        foreach (ProtocolVehicle *i, squawkMap.values()) {
            if (i->ident.uid == d.uid) {
                v = i;
                break;
            }
        }
        if (v) {
            qDebug() << "vehicle protocol exists";
            //update from ident
            if (v->squawk != squawk || memcmp(&(v->ident), &d, sizeof(d)) != 0) {
                squawkMap.remove(v->squawk);
                squawkMap.remove(squawkMap.key(v));
                squawkMap.insert(squawk, v);
                v->squawk = squawk;
                v->ident = d;
                qDebug() << "vehicle ident updated";
                v->identUpdated();
            }
        } else {
            //new Vehicle found
            v = addVehicle(squawk, d);
            emit vehicleIdentified(v);
        }
        //check squawk with uid
        if (squawkMap.contains(squawk)) {
            if (squawkMap.value(squawk) != v && d.uid != v->ident.uid) {
                //duplicate squawk came with this ident
                emit identAssigned(v, d);
                identAssign(squawk, d);
                break;
            }
        } else {
            squawkMap.insert(squawk, v);
        }
    } break;
    case mandala::cmd::env::vehicle::downlink::meta.uid: {
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

ProtocolVehicle *ProtocolVehicles::addVehicle(quint16 squawk, ProtocolVehicles::IdentData ident)
{
    qDebug() << "new vehicle protocol";
    ProtocolVehicle *v = new ProtocolVehicle(squawk, ident, this);
    return v;
}

void ProtocolVehicles::identRequest(quint16 squawk)
{
    //qDebug() << "scheduled ident req";
    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::ident::meta.uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);
    // schedule
    if (!reqList.contains(ostream.toByteArray())) {
        reqList.append(ostream.toByteArray());
        reqTimer.start();
    }
}

void ProtocolVehicles::identAssign(quint16 squawk, const IdentData &ident)
{
    qDebug() << "assign" << squawk << ident.callsign << ident.uid << ident.vclass;

    //generate squawk
    quint16 tcnt = 32767;
    do {
        squawk = (static_cast<quint16>(QRandomGenerator::global()->generate()) + tcnt) ^ squawk;
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
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::ident::meta.uid);
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

    QByteArray buid = QByteArray::fromHex(ident.uid.toUtf8());
    if (buid.size() != sizeof(dIdent.vuid)) {
        qWarning() << "wrong vehicle UID" << ident.uid;
        return;
    }
    memcpy(dIdent.vuid, buid.data(), sizeof(dIdent.vuid));

    dIdent.vclass = ident.vclass;
    dIdent.write(&ostream);

    process_uplink(ostream.toByteArray());
}

void ProtocolVehicles::send(quint16 squawk, QByteArray packet)
{
    //qDebug() << payload.toHex();
    if (!squawk) {
        //local vehicle
        process_uplink(packet);
    }
    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::uplink::meta.uid);
    ostream.write<xbus::vehicle::squawk_t>(squawk);
    ostream.append(packet);
    process_uplink(ostream.toByteArray());
}

void ProtocolVehicles::sendHeartbeat()
{
    ostream.reset();
    ostream.write<xbus::pid_t>(mandala::cmd::env::vehicle::uplink::meta.uid);
    process_uplink(ostream.toByteArray());
}
