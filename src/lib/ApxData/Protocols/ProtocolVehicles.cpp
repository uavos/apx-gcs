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

#include <ApxLink/node.h>
#include <Mandala/MandalaCore.h>
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
    uint data_cnt = packet.size();
    if (data_cnt <= bus_packet_size_hdr)
        return false;
    data_cnt -= bus_packet_size_hdr;
    _bus_packet *p = reinterpret_cast<_bus_packet *>(packet.data());
    switch (p->id) {
    case idx_xpdr: { //transponder from UAV received
        if (data_cnt != sizeof(IDENT::_xpdr))
            break;
        IDENT::_xpdr *xpdr = reinterpret_cast<IDENT::_xpdr *>(p->data);
        //qDebug()<<"xpdr"<<xpdr->squawk;
        ProtocolVehicle *v = squawkMap.value(xpdr->squawk);
        if (!v) {
            //new transponder detected, request IDENT
            identRequest(xpdr->squawk);
            break;
        }
        XpdrData d;
        d.lat = xpdr->lat;
        d.lon = xpdr->lon;
        d.alt = xpdr->alt;
        d.gSpeed = xpdr->gSpeed / 100.0;
        d.course = xpdr->crs * (180.0 / 32768.0);
        d.mode = xpdr->mode;
        v->xpdrData(d);
    } break;
    case idx_ident: {
        qDebug() << "ident received";
        if (data_cnt != sizeof(IDENT::_ident))
            break;
        IDENT::_ident *ident = reinterpret_cast<IDENT::_ident *>(p->data);
        ident->callsign[sizeof(ident->callsign) - 1] = 0;
        IdentData d;
        d.callsign = QString(static_cast<const char *>(ident->callsign));
        d.uid = QByteArray(reinterpret_cast<const char *>(ident->uid), sizeof(ident->uid))
                    .toHex()
                    .toUpper();
        d.vclass = ident->vclass;
        //emit identData(ident->squawk,d);
        if ((!ident->squawk) || d.callsign.isEmpty()) {
            //received zero SQUAWK
            identAssign(ident->squawk, d);
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
            if (v->squawk != ident->squawk || memcmp(&(v->ident), &d, sizeof(d)) != 0) {
                squawkMap.remove(v->squawk);
                squawkMap.remove(squawkMap.key(v));
                squawkMap.insert(ident->squawk, v);
                v->squawk = ident->squawk;
                v->ident = d;
                qDebug() << "vehicle ident updated";
                v->identUpdated();
            }
        } else {
            //new Vehicle found
            v = addVehicle(ident->squawk, d);
            emit vehicleIdentified(v);
        }
        //check squawk with uid
        if (squawkMap.contains(ident->squawk)) {
            if (squawkMap.value(ident->squawk) != v && d.uid != v->ident.uid) {
                //duplicate squawk came with this ident
                emit identAssigned(v, d);
                identAssign(ident->squawk, d);
                break;
            }
        } else {
            squawkMap.insert(ident->squawk, v);
        }
    } break;
    case idx_dlink: {
        if (data_cnt <= sizeof(IDENT::_squawk))
            break;
        IDENT::_squawk squawk = p->data[0] | p->data[1] << 8;
        if (!squawk)
            break; //broadcast?
        //check if new transponder detected, request IDENT
        ProtocolVehicle *v = squawkMap.value(squawk);
        if (v)
            v->unpack(packet.right(data_cnt - sizeof(IDENT::_squawk)));
        else
            identRequest(squawk);
    } break;
    default:
        local->unpack(packet);
    }
    return true;
}
//=============================================================================
ProtocolVehicle *ProtocolVehicles::addVehicle(quint64 squawk, ProtocolVehicles::IdentData ident)
{
    qDebug() << "new vehicle protocol";
    ProtocolVehicle *v = new ProtocolVehicle(squawk, ident, this);
    return v;
}
//=============================================================================
void ProtocolVehicles::identRequest(quint16 squawk)
{
    //qDebug() << "scheduled ident req";
    scheduleRequest(QByteArray()
                        .append((unsigned char) idx_ident)
                        .append((unsigned char) squawk)
                        .append((unsigned char) (squawk >> 8)));
}
//=============================================================================
void ProtocolVehicles::identAssign(quint16 squawk, const IdentData &ident)
{
    qDebug() << "assign" << squawk << ident.callsign << ident.uid << ident.vclass;
    IDENT::_ident i;
    memset(&i, 0, sizeof(i));
    //generate squawk
    uint tcnt = 1000000;
    do {
        squawk = (qrand() + tcnt) ^ squawk;
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
    i.squawk = squawk;

    //unique squawk assigned, update callsign
    QString s = ident.callsign;
    if (s.isEmpty())
        s = QString("UAVOS-%1").arg((ulong) squawk, 4, 16, QLatin1Char('0')).toUpper();
    s.truncate(sizeof(i.callsign) - 1);
    s = s.toUpper();
    memcpy(i.callsign, s.toUtf8().data(), s.size());
    QByteArray buid = QByteArray::fromHex(ident.uid.toUtf8());
    if (buid.size() != sizeof(i.uid)) {
        qWarning() << "wrong vehicle UID" << ident.uid;
        return;
    }
    memcpy(i.uid, buid.data(), sizeof(i.uid));

    i.vclass = ident.vclass;

    //send new ident
    QByteArray ba = QByteArray().append((unsigned char) idx_ident);
    int sz = ba.size();
    ba.resize(sz + sizeof(IDENT::_ident));
    memcpy(ba.data() + sz, &i, sizeof(IDENT::_ident));
    emit sendUplink(ba);
}
//=============================================================================
void ProtocolVehicles::vehicleSendUplink(quint16 squawk, QByteArray packet)
{
    emit sendUplink(QByteArray()
                        .append((unsigned char) idx_dlink)
                        .append((unsigned char) squawk)
                        .append((unsigned char) (squawk >> 8))
                        .append(packet));
}
//=============================================================================
void ProtocolVehicles::sendHeartbeat()
{
    emit sendUplink(QByteArray().append((char) idx_ping).append((char) 0));
}
//=============================================================================
