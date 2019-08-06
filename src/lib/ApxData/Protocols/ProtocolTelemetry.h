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
#ifndef ProtocolTelemetry_H
#define ProtocolTelemetry_H
#include "ProtocolBase.h"
#include <Dictionary/DictMandala.h>
#include <QtCore>
class ProtocolVehicle;
//=============================================================================
class ProtocolTelemetry : public ProtocolBase
{
    Q_OBJECT
public:
    ProtocolTelemetry(ProtocolVehicle *vehicle);

    DictMandala *mandala;

private:
    ProtocolVehicle *vehicle;
    QMap<quint16, double> values;
    uint8_t tmp[32];

    void syncValues();

private slots:
    void dlinkData(quint16 id, QByteArray data);
    void downstreamData(QByteArray data);
    void serialData(QByteArray data);

    QByteArray getPacket(quint16 pid, QByteArray payload);
    void sendUplinkValue(quint16 id, QByteArray data);

public slots:
    void sendValue(quint16 id, double v);
    void sendVectorValue(quint16 id, double v1, double v2, double v3);
    void sendPointValue(quint16 id, double v1, double v2);
    void sendValueRequest(quint16 id);

signals:
    void downstreamDataReceived();
    void valueDataReceived();
    void mandalaValueReceived(quint16 id, double v);
    void serialDataReceived(quint16 portNo, QByteArray data);
};
//=============================================================================
#endif
