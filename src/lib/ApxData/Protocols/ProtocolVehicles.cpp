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
#include "ProtocolServiceFirmware.h"
#include "ProtocolVehicle.h"

#include <Xbus/XbusPacket.h>
#include <Xbus/XbusVehicle.h>

#include <Dictionary/MandalaIndex.h>
//=============================================================================
ProtocolVehicles::ProtocolVehicles(QObject *parent)
    : ProtocolBase(parent)
    , txbuf(xbus::size_packet_max, '\0')
{
    IdentData ident;
    ident.vclass = 0;
    local = new ProtocolVehicle(0, ident, this);
    firmware = new ProtocolServiceFirmware(local->service);
}
//=============================================================================
bool ProtocolVehicles::unpack(QByteArray packet)
{
    if (packet.size() > xbus::size_packet_max)
        return false;
    uint16_t psize = static_cast<uint16_t>(packet.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());

    XbusStreamReader stream(pdata, psize);
    if (stream.tail() < sizeof(xbus::pid_t))
        return false;

    xbus::pid_t pid = stream.read<xbus::pid_t>();
    switch (pid) {
    case mandala::idx_xpdr: { //transponder from UAV received
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (xbus::vehicle::Xpdr::psize() != stream.tail())
            break;

        ProtocolVehicle *v = squawkMap.value(squawk);
        if (!v) {
            //new transponder detected, request IDENT
            identRequest(squawk);
            break;
        }

        xbus::vehicle::Xpdr dXpdr;
        dXpdr.read(&stream);

        XpdrData d;
        d.lat = static_cast<double>(dXpdr.lat);
        d.lon = static_cast<double>(dXpdr.lon);
        d.alt = static_cast<double>(dXpdr.alt);
        d.gSpeed = static_cast<double>(dXpdr.speed);
        d.course = static_cast<double>(dXpdr.course);
        d.mode = dXpdr.mode;
        v->xpdrData(d);
    } break;
    case mandala::idx_ident: {
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        qDebug() << "ident received" << squawk;
        if (xbus::vehicle::Ident::psize() != stream.tail())
            break;

        xbus::vehicle::Ident dIdent;
        dIdent.read(&stream);

        IdentData d;
        d.callsign = QString(QByteArray(dIdent.callsign.data(), dIdent.callsign.size())).trimmed();
        d.uid = QByteArray(reinterpret_cast<const char *>(dIdent.vuid.data()), dIdent.vuid.size())
                    .toHex()
                    .toUpper();
        d.vclass = dIdent.vclass;
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
    case mandala::idx_dlink: {
        const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
        if (stream.tail() == 0)
            break;

        if (!squawk)
            break; //broadcast?
        //check if new transponder detected, request IDENT
        ProtocolVehicle *v = squawkMap.value(squawk);
        if (v)
            v->unpack(packet.mid(stream.position()));
        else
            identRequest(squawk);
    } break;
    default:
        local->unpack(packet);
    }
    return true;
}
//=============================================================================
ProtocolVehicle *ProtocolVehicles::addVehicle(quint16 squawk, ProtocolVehicles::IdentData ident)
{
    qDebug() << "new vehicle protocol";
    ProtocolVehicle *v = new ProtocolVehicle(squawk, ident, this);
    return v;
}
//=============================================================================
void ProtocolVehicles::identRequest(quint16 squawk)
{
    //qDebug() << "scheduled ident req";
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(mandala::idx_ident);
    stream.write<xbus::vehicle::squawk_t>(squawk);
    scheduleRequest(txbuf.left(stream.position()));
}
//=============================================================================
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

    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(mandala::idx_ident);
    stream.write<xbus::vehicle::squawk_t>(squawk);

    xbus::vehicle::Ident dIdent;

    //unique squawk assigned, update callsign
    QString s = ident.callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(static_cast<ulong>(squawk), 4, 16, QLatin1Char('0')).toUpper();
    s.truncate(sizeof(dIdent.callsign) - 1);
    s = s.toUpper();
    dIdent.callsign.fill(0);
    std::copy(s.toUtf8().begin(), s.toUtf8().end(), dIdent.callsign.begin());

    QByteArray buid = QByteArray::fromHex(ident.uid.toUtf8());
    if (buid.size() != sizeof(dIdent.vuid)) {
        qWarning() << "wrong vehicle UID" << ident.uid;
        return;
    }
    std::copy(buid.begin(), buid.end(), dIdent.vuid.begin());

    dIdent.vclass = ident.vclass;

    //update payload
    dIdent.write(&stream);

    //send new ident
    emit sendUplink(txbuf.left(stream.position()));
}
//=============================================================================
void ProtocolVehicles::vehicleSendUplink(quint16 squawk, QByteArray payload)
{
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(mandala::idx_dlink);
    stream.write<xbus::vehicle::squawk_t>(squawk);
    emit sendUplink(txbuf.left(stream.position()).append(payload));
}
//=============================================================================
void ProtocolVehicles::sendHeartbeat()
{
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(mandala::idx_ping);
    emit sendUplink(txbuf.left(stream.position()).append('\0'));
}
//=============================================================================
