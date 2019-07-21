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

#include <Xbus/XbusNode.h>
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
    if (packet.size() > XbusPacket::size_packet)
        return false;
    uint16_t psize = static_cast<uint16_t>(packet.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusPacket p(pdata);
    if (!p.isValid(psize))
        return false;
    QByteArray payload(packet.right(p.payloadSize(psize)));

    switch (p.pid()) {
    default:
        emit dlinkData(p.pid(), payload);
        break;
    case mandala::idx_downstream:
        emit downstreamData(payload);
        break;
    case mandala::idx_data:
        emit serialData(payload);
        break;
    case mandala::idx_mission:
        emit missionData(payload);
        break;
    case mandala::idx_service: {
        XbusNode pNode(pdata);
        XbusNode::Header hdr;
        pNode.read(hdr);

        if (!pNode.isValid(psize))
            return false;
        if (!pNode.isValid(psize)) {
            if (pNode.payloadSize(psize + 2) != 1)
                break;
            hdr.cmd = XbusNode::apc_search; //assume search cmd if just uid received
        }
        QString sn(QByteArray(reinterpret_cast<const char *>(hdr.guid.data()), hdr.guid.size())
                       .toHex()
                       .toUpper());
        //qDebug()<<"idx_service"<<sn;
        if (!(sn.isEmpty() || sn.count('0') == sn.size())) {
            emit serviceData(sn, hdr.cmd, packet.right(pNode.payloadSize(psize)));
        }
    } break;
    }
    return true;
}
//=============================================================================
void ProtocolVehicle::sendRequest(quint8 pid, QByteArray payload)
{
    QByteArray packet(XbusPacket(nullptr).payloadOffset(), 0);
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusPacket p(pdata);
    p.setPid(pid);
    packet.append(payload);
    emit sendUplink(packet);
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
void ProtocolVehicle::sendServiceRequest(QString sn, quint16 cmd, QByteArray payload)
{
    QByteArray packet(XbusNode(nullptr).payloadOffset(), 0);
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());
    XbusNode pNode(pdata);
    pNode.setPid(mandala::idx_service);
    XbusNode::Header hdr;
    if (sn.isEmpty()) {
        std::fill(hdr.guid.begin(), hdr.guid.end(), 0);
    } else {
        QByteArray src(QByteArray::fromHex(sn.toUtf8()));
        std::copy(src.begin(), src.end(), hdr.guid.begin());
    }
    hdr.cmd = static_cast<XbusNode::cmd_t>(cmd);
    pNode.write(hdr);
    packet.append(payload);
    emit sendUplink(packet);
}
//=============================================================================
