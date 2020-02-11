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

#include <Mandala/MandalaMetaTree.h>

#include <Xbus/XbusNode.h>
#include <Xbus/XbusPacket.h>
//=============================================================================
ProtocolVehicle::ProtocolVehicle(quint16 squawk,
                                 ProtocolVehicles::IdentData ident,
                                 ProtocolVehicles *vehicles)
    : ProtocolBase(vehicles)
    , squawk(squawk)
    , ident(ident)
    , vehicles(vehicles)
    , txbuf(xbus::size_packet_max, '\0')
{
    if (vehicles) {
        if (squawk) {
            connect(this, &ProtocolVehicle::uplinkData, this, [this](QByteArray packet) {
                this->vehicles->vehicleSendUplink(this->squawk, packet);
            });
        } else {
            //local
            connect(this, &ProtocolVehicle::uplinkData, vehicles, &ProtocolVehicles::send);
        }
    }

    mission = new ProtocolMission(this);
    service = new ProtocolService(this);
}
//=============================================================================
void ProtocolVehicle::unpack(const QByteArray packet)
{
    //packet might be unwrapped from <cmd::vehicle::xxx>
    uint16_t psize = static_cast<uint16_t>(packet.size());
    const uint8_t *pdata = reinterpret_cast<const uint8_t *>(packet.data());

    XbusStreamReader stream(pdata, psize);
    if (stream.tail() < sizeof(xbus::pid_t)) {
        qWarning() << "packet" << packet.toHex().toUpper();
        return;
    }

    xbus::pid_t pid = stream.read<xbus::pid_t>();
    QByteArray payload(packet.mid(stream.position()));

    //qDebug() << ident.callsign << pid;

    switch (pid) {
    default:
        emit receivedData(pid, payload);
        break;
    case mandala::cmd::env::telemetry::data::meta.uid:
        emit telemetryData(payload);
        break;
    case mandala::cmd::env::vcp::rx::meta.uid:
        if (payload.size() < 2) {
            qWarning() << "Empty serial RX data received";
            break;
        }
        emit serialRxData(static_cast<quint8>(payload.at(0)), payload.right(payload.size() - 1));
        break;
    case mandala::cmd::env::vcp::tx::meta.uid:
        if (payload.size() < 2) {
            qWarning() << "Empty serial TX data received";
            break;
        }
        emit serialTxData(static_cast<quint8>(payload.at(0)), payload.right(payload.size() - 1));
        break;
    case mandala::cmd::env::mission::data::meta.uid:
        emit missionData(payload);
        break;
    case mandala::cmd::env::script::jsexec::meta.uid:
        emit jsexecData(payload);
        break;

    case mandala::cmd::env::nmt::meta.uid: {
        if (stream.tail() < sizeof(xbus::node::guid_t))
            return;
        xbus::node::guid_t guid;
        memset(guid, 0, sizeof(guid));
        stream.read(guid, sizeof(guid));

        xbus::node::cmd_t cmd;
        if (stream.tail() < sizeof(xbus::node::cmd_t)) {
            cmd = xbus::node::apc_search;
            stream.reset(stream.position() + stream.tail());
        } else
            cmd = stream.read<xbus::node::cmd_t>();

        QString sn(QByteArray(reinterpret_cast<const char *>(guid), sizeof(guid)).toHex().toUpper());
        //qDebug() << "nmt" << sn;
        if (!(sn.isEmpty() || sn.count('0') == sn.size())) {
            emit serviceData(sn, cmd, packet.mid(stream.position()));
        }
    } break;
    }
}
//=============================================================================
void ProtocolVehicle::sendRequest(quint16 pid, QByteArray payload)
{
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(pid);
    send(txbuf.left(stream.position()).append(payload));
}
//=============================================================================
void ProtocolVehicle::vmexec(QString func)
{
    emit sendRequest(mandala::cmd::env::script::vmexec::meta.uid, func.toUtf8());
}
void ProtocolVehicle::sendSerial(quint8 portID, QByteArray data)
{
    emit sendRequest(mandala::cmd::env::vcp::tx::meta.uid,
                     QByteArray().append(static_cast<char>(portID)).append(data));
}
void ProtocolVehicle::sendMissionRequest(QByteArray data)
{
    emit sendRequest(mandala::cmd::env::mission::data::meta.uid, data);
}
void ProtocolVehicle::sendServiceRequest(QString sn, quint16 cmd, QByteArray payload)
{
    //qDebug() << cmd << sn << this << payload.toHex();
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(txbuf.data()));
    stream.write<xbus::pid_t>(mandala::cmd::env::nmt::meta.uid);
    xbus::node::guid_t guid;
    memset(guid, 0, sizeof(guid));
    if (!sn.isEmpty()) {
        QByteArray src(QByteArray::fromHex(sn.toUtf8()));
        size_t sz = static_cast<size_t>(src.size());
        if (sz > sizeof(guid)) {
            sz = sizeof(guid);
            qWarning() << "guid size:" << sz;
        }
        memcpy(guid, src.data(), sz);
    }
    stream.write(guid, sizeof(guid));
    stream.write<xbus::node::cmd_t>(cmd);

    send(txbuf.left(stream.position()).append(payload));
}
//=============================================================================
