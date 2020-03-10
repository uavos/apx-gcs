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
#pragma once

#include "ProtocolBase.h"
#include "ProtocolNodes.h"
#include "ProtocolVehicles.h"

#include <QtCore>

class ProtocolVehicle : public ProtocolBase
{
    Q_OBJECT
public:
    ProtocolVehicle(xbus::vehicle::squawk_t squawk,
                    const xbus::vehicle::ident_s &ident,
                    ProtocolVehicles *vehicles);

    void downlink(ProtocolStreamReader &stream);

    xbus::vehicle::squawk_t squawk;
    xbus::vehicle::ident_s ident;

    ProtocolVehicles *vehicles;
    ProtocolNodes *nodes;

private:
    uint8_t txbuf[xbus::size_packet_max];
    ProtocolStreamWriter ostream{txbuf, sizeof(txbuf)};

public slots:
    void send(const QByteArray packet);
    void vmexec(QString func);
    void sendSerial(quint8 portID, QByteArray data);

signals:
    void xpdrData(const xbus::vehicle::xpdr_s &xpdr);
    void identUpdated();

    //known received data
    void telemetryData(ProtocolStreamReader *stream);

    void serialRxData(quint16 portNo, QByteArray data);
    void serialTxData(quint16 portNo, QByteArray data);

    void jsexecData(QString script);
    void missionData(QByteArray data);

    //data with other id (i.e. mandala data)
    void receivedData(xbus::pid_t pid, ProtocolStreamReader *stream);
};
