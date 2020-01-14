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

#include <Dictionary/MandalaIndex.h>
#include <Xbus/XbusNode.h>
#include <Xbus/XbusPacket.h>
//=============================================================================
ProtocolVehicle::ProtocolVehicle(quint16 squawk,
                                 ProtocolVehicles::IdentData ident,
                                 ProtocolVehicles *vehicles)
    : ProtocolBase(vehicles)
    , squawk(squawk)
    , ident(ident)
    , txbuf(xbus::size_packet_max, '\0')
{
    if (vehicles) {
        if (squawk) {
            connect(this, &ProtocolVehicle::sendUplink, this, [this, vehicles](QByteArray packet) {
                vehicles->vehicleSendUplink(this->squawk, packet);
            });
        } else {
            //local
            connect(this, &ProtocolVehicle::sendUplink, vehicles, &ProtocolVehicles::sendUplink);
        }
    }

    telemetry = new ProtocolTelemetry(this);
    mission = new ProtocolMission(this);
    service = new ProtocolService(this);
}
//=============================================================================
bool ProtocolVehicle::unpack(QByteArray packet)
{
    uint16_t psize = static_cast<uint16_t>(packet.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(packet.data());

    XbusStreamReader stream(pdata, psize);
    if (stream.tail() < sizeof(xbus::pid_t))
        return false;

    xbus::pid_t pid = stream.read<xbus::pid_t>();
    QByteArray payload(packet.mid(stream.position()));

    //qDebug() << ident.callsign << pid;

    switch (pid) {
    default:
        emit dlinkData(pid, payload);
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
    case mandala::idx_jsexec:
        emit jsexecData(payload);
        break;

    case mandala::idx_service: {
        if (stream.tail() < sizeof(xbus::node::guid_t))
            return false;
        xbus::node::guid_t guid = stream.read<xbus::node::guid_t>();

        xbus::node::cmd_t cmd;
        if (stream.tail() < sizeof(xbus::node::cmd_t))
            cmd = xbus::node::apc_search;
        else
            cmd = stream.read<xbus::node::cmd_t>();

        QString sn(
            QByteArray(reinterpret_cast<const char *>(guid.data()), guid.size()).toHex().toUpper());
        //qDebug() << "idx_service" << sn;
        if (!(sn.isEmpty() || sn.count('0') == sn.size())) {
            emit serviceData(sn, cmd, packet.mid(stream.position()));
        }
    } break;
    }
    return true;
}
//=============================================================================
void ProtocolVehicle::sendRequest(quint8 pid, QByteArray payload)
{
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(pid);
    emit sendUplink(txbuf.left(stream.position()).append(payload));
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
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(mandala::idx_service);
    xbus::node::guid_t guid;
    if (sn.isEmpty()) {
        std::fill(guid.begin(), guid.end(), 0);
    } else {
        QByteArray src(QByteArray::fromHex(sn.toUtf8()));
        std::copy(src.begin(), src.end(), guid.begin());
    }
    stream.write(guid);
    stream.write<xbus::node::cmd_t>(cmd);

    emit sendUplink(txbuf.left(stream.position()).append(payload));
}
//=============================================================================
