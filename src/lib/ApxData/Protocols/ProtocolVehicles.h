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
#include "ProtocolServiceFirmware.h"
#include <QtCore>

class ProtocolVehicle;
class ProtocolConverter;

class ProtocolVehicles : public ProtocolBase
{
    Q_OBJECT
public:
    ProtocolVehicles(QObject *parent = nullptr);

    friend class ProtocolVehicle;

    struct XpdrData
    {
        double lat;
        double lon;
        double alt;
        double gSpeed;
        double course;
        quint8 mode;
    };
    struct IdentData
    {
        QString callsign;
        QString uid;
        uint8_t vclass;
    };

    ProtocolVehicle *local;
    //ProtocolServiceFirmware *firmware;

    void setConverter(ProtocolConverter *c);
    ProtocolConverter *converter() const;

private:
    ProtocolConverter *m_converter{nullptr};

    QTimer reqTimer;
    QList<QByteArray> reqList;

    QMap<quint16, ProtocolVehicle *> squawkMap;

    uint8_t txbuf[xbus::size_packet_max];
    ProtocolStreamWriter ostream{txbuf, sizeof(txbuf)};

    void send(quint16 squawk, QByteArray packet);

    ProtocolVehicle *addVehicle(quint16 squawk, ProtocolVehicles::IdentData ident);
    void identRequest(quint16 squawk);
    void identAssign(quint16 squawk, const ProtocolVehicles::IdentData &ident);

private slots:
    void process_downlink(const QByteArray packet);
    void process_uplink(const QByteArray packet); //call to send data to tx

public slots:
    void sendHeartbeat();

signals:
    void vehicleIdentified(ProtocolVehicle *protocol);
    void identAssigned(ProtocolVehicle *v, const IdentData &ident);

    // data comm
public slots:
    void downlink(const QByteArray packet); //connect rx
signals:
    void uplink(const QByteArray packet); //connect tx
};
