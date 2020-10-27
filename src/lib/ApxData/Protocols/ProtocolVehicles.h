/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "ProtocolBase.h"
#include "ProtocolTrace.h"

#include <xbus/XbusVehicle.h>

#include <QtCore>

class ProtocolVehicle;
class ProtocolConverter;
class ProtocolNode;
class ProtocolNodes;

class ProtocolVehicles : public ProtocolBase
{
    Q_OBJECT
    Q_PROPERTY(ProtocolTrace *trace MEMBER m_trace CONSTANT)

public:
    ProtocolVehicles(QObject *parent = nullptr);

    friend class ProtocolVehicle;

    ProtocolVehicle *local;
    ProtocolVehicle *replay;

    void setConverter(ProtocolConverter *c);
    ProtocolConverter *converter() const;

private:
    ProtocolConverter *m_converter{nullptr};
    ProtocolTrace *m_trace;

    QTimer reqTimer;
    QList<QByteArray> reqList;

    QMap<xbus::vehicle::squawk_t, ProtocolVehicle *> squawkMap;

    uint8_t txbuf[xbus::size_packet_max];
    ProtocolStreamWriter ostream{txbuf, sizeof(txbuf)};

    void send(xbus::vehicle::squawk_t squawk, QByteArray packet);

    ProtocolVehicle *addVehicle(xbus::vehicle::squawk_t squawk,
                                const xbus::vehicle::ident_s &ident,
                                const QString &callsign);
    void identRequest(xbus::vehicle::squawk_t squawk);
    void identAssign(const xbus::vehicle::ident_s &ident, const QString &callsign);

private slots:
    void process_downlink(const QByteArray packet);
    void process_uplink(const QByteArray packet); //call to send data to tx

public slots:
    void sendHeartbeat();

signals:
    void vehicleIdentified(ProtocolVehicle *protocol);
    void nodeNotify(ProtocolNode *protocol); // forwarded from all vehicles

    void stopNmtRequests();

    // data comm
public slots:
    void downlink(const QByteArray packet); //connect rx
signals:
    void uplink(const QByteArray packet); //connect tx
};
