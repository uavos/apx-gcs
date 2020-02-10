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
#ifndef ProtocolVehicle_H
#define ProtocolVehicle_H
#include "ProtocolBase.h"
#include "ProtocolMission.h"
#include "ProtocolService.h"
#include "ProtocolVehicles.h"
#include <QtCore>
//=============================================================================
class ProtocolVehicle : public ProtocolBase
{
    Q_OBJECT
public:
    ProtocolVehicle(quint16 squawk, ProtocolVehicles::IdentData ident, ProtocolVehicles *vehicles);

    quint16 squawk;
    ProtocolVehicles::IdentData ident;

    ProtocolVehicles *vehicles;
    ProtocolMission *mission;
    ProtocolService *service;

private:
    QByteArray txbuf;

    void sendRequest(quint16 pid, QByteArray payload);
    void unpack(const QByteArray packet) override;

public slots:

    void vmexec(QString func);
    void sendSerial(quint8 portID, QByteArray data);
    void sendMissionRequest(QByteArray data = QByteArray());
    void sendServiceRequest(QString sn, quint16 cmd, QByteArray payload);

signals:
    void xpdrData(const ProtocolVehicles::XpdrData &xpdr);
    void identUpdated();

    //known received data
    void telemetryData(QByteArray data);

    void serialRxData(quint16 portNo, QByteArray data);
    void serialTxData(quint16 portNo, QByteArray data);

    void jsexecData(QByteArray data);
    void missionData(QByteArray data);
    void serviceData(QString sn, quint16 cmd, QByteArray data);

    //data with other id (i.e. mandala data)
    void receivedData(quint16 id, QByteArray data);
};
//=============================================================================
#endif
