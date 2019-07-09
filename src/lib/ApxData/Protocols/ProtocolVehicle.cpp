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
#include "ProtocolVehicle.h"

#include <Xbus/xbus_node.h>
#include <Dictionary/MandalaIndex.h>
//=============================================================================
ProtocolVehicle::ProtocolVehicle(quint16 squawk,
                                 ProtocolVehicles::IdentData ident,
                                 ProtocolVehicles *vehicles)
    : ProtocolBase(vehicles)
    , squawk(squawk)
    , ident(ident)
{
    if (squawk) {
        connect(this, &ProtocolVehicle::sendUplink, this, [this, vehicles](QByteArray packet) {
            vehicles->vehicleSendUplink(this->squawk, packet);
        });
    } else {
        //local
        connect(this, &ProtocolVehicle::sendUplink, vehicles, &ProtocolVehicles::sendUplink);
    }

    telemetry = new ProtocolTelemetry(this);
    mission = new ProtocolMission(this);
    service = new ProtocolService(this);
}
//=============================================================================
bool ProtocolVehicle::unpack(QByteArray packet)
{
    size_t psize = packet.size();
    if (psize <= sizeof(xbus::hdr_t))
        return false;
    const xbus::hdr_t *p = reinterpret_cast<const xbus::hdr_t *>(packet.data());
    QByteArray data(packet.right(psize - sizeof(xbus::hdr_t)));
    switch (p->pid) {
    default:
        emit dlinkData(p->pid, data);
        break;
    case mandala::idx_downstream:
        emit downstreamData(data);
        break;
    case mandala::idx_data:
        emit serialData(data);
        break;
    case mandala::idx_mission:
        emit missionData(data);
        break;
    case mandala::idx_service: {
        quint16 cmd = xbus::apc_search;
        const xbus::hdr_node_t *hdr = reinterpret_cast<const xbus::hdr_node_t *>(p);
        if (data.size() >= static_cast<int>(sizeof(xbus::hdr_node_t)))
            cmd = hdr->cmd;
        else if (data.size() < (static_cast<int>(sizeof(xbus::hdr_node_t)) - 1))
            break;
        QString sn(QByteArray(reinterpret_cast<const char *>(hdr->guid), sizeof(xbus::node_guid_t))
                       .toHex()
                       .toUpper());
        //qDebug()<<"idx_service"<<sn;
        if (!(sn.isEmpty() || sn.count('0') == sn.size())) {
            emit serviceData(sn, hdr->cmd, packet.right(psize - sizeof(xbus::hdr_node_t)));
        }
    } break;
    }
    return true;
}
//=============================================================================
void ProtocolVehicle::sendRequest(quint8 id, QByteArray data)
{
    emit sendUplink(QByteArray().append(static_cast<char>(id)).append(data));
}
//=============================================================================
void ProtocolVehicle::vmexec(QString func)
{
    emit sendRequest(mandala::idx_vmexec, func.toUtf8());
}
void ProtocolVehicle::sendSerial(quint8 portID, QByteArray data)
{
    emit sendRequest(mandala::idx_data, QByteArray().append(static_cast<char>(portID)).append(data));
}
void ProtocolVehicle::sendMissionRequest(QByteArray data)
{
    emit sendRequest(mandala::idx_mission, data);
}
void ProtocolVehicle::sendServiceRequest(QString sn, quint16 cmd, QByteArray data)
{
    QByteArray pdata = QByteArray::fromHex(sn.toUtf8());
    if (pdata.isEmpty())
        pdata = QByteArray(sizeof(xbus::node_guid_t), static_cast<char>(0));
    pdata.append(static_cast<char>(cmd)).append(data);
    //qDebug()<<pdata.toHex().toUpper();
    emit sendRequest(mandala::idx_service, pdata);
}
//=============================================================================
