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

#include <node.h>
#include <Mandala.h>
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
    int data_cnt = packet.size();
    if (data_cnt <= bus_packet_size_hdr)
        return false;
    data_cnt -= bus_packet_size_hdr;
    const _bus_packet *p = reinterpret_cast<const _bus_packet *>(packet.data());
    QByteArray data(reinterpret_cast<const char *>(p->data), data_cnt);
    switch (p->id) {
    default:
        emit dlinkData(p->id, data);
        break;
    case idx_downstream:
        emit downstreamData(data);
        break;
    case idx_data:
        emit serialData(data);
        break;
    case idx_mission:
        emit missionData(data);
        break;
    case idx_service: {
        quint16 cmd = apc_search;
        if (data.size() >= static_cast<int>(bus_packet_size_hdr_srv))
            cmd = p->srv.cmd;
        else if (data.size() < (static_cast<int>(bus_packet_size_hdr_srv) - 1))
            break;
        QString sn(QByteArray(reinterpret_cast<const char *>(p->srv.sn), sizeof(_node_sn))
                       .toHex()
                       .toUpper());
        //qDebug()<<"idx_service"<<sn;
        if (!(sn.isEmpty() || sn.count('0') == sn.size())) {
            emit serviceData(sn,
                             p->srv.cmd,
                             QByteArray(reinterpret_cast<const char *>(p->srv.data),
                                        packet.size() - static_cast<int>(bus_packet_size_hdr_srv)));
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
    emit sendRequest(idx_vmexec, func.toUtf8());
}
void ProtocolVehicle::sendSerial(quint8 portID, QByteArray data)
{
    emit sendRequest(idx_data, QByteArray().append(static_cast<char>(portID)).append(data));
}
void ProtocolVehicle::sendMissionRequest(QByteArray data)
{
    emit sendRequest(idx_mission, data);
}
void ProtocolVehicle::sendServiceRequest(QString sn, quint16 cmd, QByteArray data)
{
    QByteArray pdata = QByteArray::fromHex(sn.toUtf8());
    if (pdata.isEmpty())
        pdata = QByteArray(sizeof(_node_sn), static_cast<char>(0));
    pdata.append(static_cast<char>(cmd)).append(data);
    //qDebug()<<pdata.toHex().toUpper();
    emit sendRequest(idx_service, pdata);
}
//=============================================================================
