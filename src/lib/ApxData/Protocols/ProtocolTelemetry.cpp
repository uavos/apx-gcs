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
#include "ProtocolTelemetry.h"
#include "ProtocolVehicle.h"

#include <Xbus/XbusPacket.h>

#include <Dictionary/MandalaIndex.h>
//=============================================================================
ProtocolTelemetry::ProtocolTelemetry(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle)
    , vehicle(vehicle)
{
    connect(this, &ProtocolTelemetry::sendUplink, vehicle, &ProtocolVehicle::sendUplink);
    connect(vehicle, &ProtocolVehicle::downstreamData, this, &ProtocolTelemetry::downstreamData);
    connect(vehicle, &ProtocolVehicle::dlinkData, this, &ProtocolTelemetry::dlinkData);
    connect(vehicle, &ProtocolVehicle::serialData, this, &ProtocolTelemetry::serialData);

    //create values cache
    mandala = new DictMandala(this);
    for (int i = 0; i < mandala->items.size(); ++i) {
        const DictMandala::Entry &d = mandala->items.at(i);
        values.insert(d.id, 0);
    }
}
//=============================================================================
void ProtocolTelemetry::syncValues()
{
    for (int i = 0; i < mandala->items.size(); ++i) {
        const DictMandala::Entry &d = mandala->items.at(i);
        double v = mandala->readValue(d);
        if (std::abs(values.value(d.id) - v) < std::numeric_limits<double>().epsilon())
            continue;
        values[d.id] = v;
        emit mandalaValueReceived(d.id, v);
    }
}
//=============================================================================
//=============================================================================
//=============================================================================
QByteArray ProtocolTelemetry::getPacket(quint16 pid, QByteArray payload)
{
    QByteArray packet(sizeof(xbus::pid_t), 0);
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(packet.data()));
    stream.write<xbus::pid_t>(pid);
    packet.append(payload);
    return packet;
}
void ProtocolTelemetry::sendUplinkValue(quint16 id, QByteArray data)
{
    emit sendUplink(getPacket(mandala::idx_uplink, getPacket(id, data)));
}
void ProtocolTelemetry::sendValue(quint16 id, double v)
{
    //qDebug()<<"sendValue"<<id<<v;
    values[id] = v;
    const DictMandala::Entry &i = mandala->items.value(mandala->idPos.value(id));
    if (!i.id)
        return;
    if (i.send_set) {
        sendUplinkValue(mandala::idx_set, mandala->packSetValue(i, v));
    } else {
        sendUplinkValue(i.id, mandala->packValue(i, v));
    }
    //qDebug()<<"sendValue"<<id<<v<<mandala->packValue(i,v).toHex();
}
void ProtocolTelemetry::sendVectorValue(quint16 id, double v1, double v2, double v3)
{
    const DictMandala::Entry &i = mandala->items.value(mandala->idPos.value(id));
    if (!i.id)
        return;
    sendUplinkValue(i.id, mandala->packVectorValue(i, v1, v2, v3));
}
void ProtocolTelemetry::sendPointValue(quint16 id, double v1, double v2)
{
    const DictMandala::Entry &i = mandala->items.value(mandala->idPos.value(id));
    if (!i.id)
        return;
    sendUplinkValue(i.id, mandala->packPointValue(i, v1, v2));
}
void ProtocolTelemetry::sendValueRequest(quint16 id)
{
    //qDebug()<<"sendValueRequest"<<id;
    sendUplinkValue(id, QByteArray());
}
//=============================================================================
//=============================================================================
void ProtocolTelemetry::dlinkData(quint16 id, QByteArray data)
{
    if (!mandala->unpackValue(id, data)) {
        //qDebug() << "unpack failed" << id << data.size();
        return;
    }
    syncValues();
    emit valueDataReceived();
}
//=============================================================================
void ProtocolTelemetry::downstreamData(QByteArray data)
{
    if (data.size() < 4)
        return;
    if (!mandala->unpackStream(data))
        return;
    syncValues();
    emit downstreamDataReceived();
}
//=============================================================================
void ProtocolTelemetry::serialData(QByteArray data)
{
    if (data.size() < 2) {
        qDebug() << "Empty serial data received";
        return;
    }
    emit serialDataReceived(static_cast<quint8>(data.at(0)), data.right(data.size() - 1));
}
//=============================================================================
//=============================================================================
