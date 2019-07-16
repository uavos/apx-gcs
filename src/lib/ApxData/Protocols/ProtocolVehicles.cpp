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
#include "ProtocolVehicle.h"
#include "ProtocolServiceFirmware.h"

#include <Xbus/XbusVehicle.h>
#include <Xbus/XbusVehicleXpdr.h>
#include <Xbus/XbusVehicleIdent.h>

#include <Dictionary/MandalaIndex.h>
//=============================================================================
ProtocolVehicles::ProtocolVehicles(QObject *parent)
    : ProtocolBase(parent)
{
    IdentData ident;
    ident.vclass = 0;
    local = new ProtocolVehicle(0, ident, this);
    firmware = new ProtocolServiceFirmware(local->service);
}
//=============================================================================
bool ProtocolVehicles::unpack(QByteArray packet)
{
    if (packet.size() > XbusPacket::size_packet)
        return false;
    uint16_t psize = static_cast<uint16_t>(packet.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusPacket p(pdata);
    if (!p.isValid(psize))
        return false;

    switch (p.pid()) {
    case mandala::idx_xpdr: { //transponder from UAV received
        XbusVehicleXpdr pXpdr(pdata);
        if (!pXpdr.isValid(psize))
            break;
        ProtocolVehicle *v = squawkMap.value(pXpdr.squawk());
        if (!v) {
            //new transponder detected, request IDENT
            identRequest(pXpdr.squawk());
            break;
        }
        XbusVehicleXpdr::Xpdr dXpdr;
        pXpdr.read(dXpdr);

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
        qDebug() << "ident received";
        XbusVehicleIdent pIdent(pdata);
        if (!pIdent.isValid(psize))
            break;

        XbusVehicleIdent::Ident dIdent;
        pIdent.read(dIdent);

        IdentData d;
        d.callsign = QString(QByteArray(dIdent.callsign.data(), dIdent.callsign.size()));
        d.uid = QByteArray(reinterpret_cast<const char *>(dIdent.vuid.data()), dIdent.vuid.size())
                    .toHex()
                    .toUpper();
        d.vclass = dIdent.vclass;
        //emit identData(ident->squawk,d);
        if ((!pIdent.squawk()) || d.callsign.isEmpty()) {
            //received zero SQUAWK
            identAssign(pIdent.squawk(), d);
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
            if (v->squawk != pIdent.squawk() || memcmp(&(v->ident), &d, sizeof(d)) != 0) {
                squawkMap.remove(v->squawk);
                squawkMap.remove(squawkMap.key(v));
                squawkMap.insert(pIdent.squawk(), v);
                v->squawk = pIdent.squawk();
                v->ident = d;
                qDebug() << "vehicle ident updated";
                v->identUpdated();
            }
        } else {
            //new Vehicle found
            v = addVehicle(pIdent.squawk(), d);
            emit vehicleIdentified(v);
        }
        //check squawk with uid
        if (squawkMap.contains(pIdent.squawk())) {
            if (squawkMap.value(pIdent.squawk()) != v && d.uid != v->ident.uid) {
                //duplicate squawk came with this ident
                emit identAssigned(v, d);
                identAssign(pIdent.squawk(), d);
                break;
            }
        } else {
            squawkMap.insert(pIdent.squawk(), v);
        }
    } break;
    case mandala::idx_dlink: {
        XbusVehicle pVehicle(pdata);
        if (!pVehicle.isValid(psize))
            break;
        if (pVehicle.isRequest(psize))
            break;

        if (!pVehicle.squawk())
            break; //broadcast?
        //check if new transponder detected, request IDENT
        ProtocolVehicle *v = squawkMap.value(pVehicle.squawk());
        if (v)
            v->unpack(packet.right(pVehicle.payloadSize(psize)));
        else
            identRequest(pVehicle.squawk());
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
    QByteArray packet(XbusVehicle(nullptr).payloadOffset(), 0);
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusVehicle pVehicle(pdata);
    pVehicle.setPid(mandala::idx_ident);
    pVehicle.setSquawk(squawk);
    scheduleRequest(packet);
}
//=============================================================================
void ProtocolVehicles::identAssign(quint16 squawk, const IdentData &ident)
{
    qDebug() << "assign" << squawk << ident.callsign << ident.uid << ident.vclass;
    QByteArray packet(XbusVehicleIdent(nullptr).payloadOffset() + sizeof(XbusVehicleIdent::Ident),
                      0);
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusVehicleIdent pIdent(pdata);

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
    pIdent.setPid(mandala::idx_ident);
    pIdent.setSquawk(squawk);

    XbusVehicleIdent::Ident dIdent;

    //unique squawk assigned, update callsign
    QString s = ident.callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg(static_cast<ulong>(squawk), 4, 16, QLatin1Char('0')).toUpper();
    s.truncate(sizeof(dIdent.callsign) - 1);
    s = s.toUpper();
    QByteArray sa(s.toUtf8());
    std::copy(sa.begin(), sa.end(), dIdent.callsign.begin());
    QByteArray buid = QByteArray::fromHex(ident.uid.toUtf8());
    if (buid.size() != sizeof(dIdent.vuid)) {
        qWarning() << "wrong vehicle UID" << ident.uid;
        return;
    }
    std::copy(buid.begin(), buid.end(), dIdent.vuid.begin());

    dIdent.vclass = ident.vclass;

    //send new ident
    pIdent.write(dIdent);
    emit sendUplink(packet);
}
//=============================================================================
void ProtocolVehicles::vehicleSendUplink(quint16 squawk, QByteArray payload)
{
    QByteArray packet(XbusVehicle(nullptr).payloadOffset(), 0);
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusVehicle pVehicle(pdata);
    pVehicle.setPid(mandala::idx_dlink);
    pVehicle.setSquawk(squawk);
    packet.append(payload);
    emit sendUplink(packet);
}
//=============================================================================
void ProtocolVehicles::sendHeartbeat()
{
    QByteArray packet(XbusPacket(nullptr).payloadOffset(), 0);
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusPacket p(pdata);
    p.setPid(mandala::idx_ping);
    packet.append('\0');
    emit sendUplink(packet);
}
//=============================================================================
