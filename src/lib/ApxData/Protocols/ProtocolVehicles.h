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
#ifndef ProtocolVehicles_H
#define ProtocolVehicles_H
#include <QtCore>
#include "ProtocolBase.h"
#include "ProtocolServiceFirmware.h"
class ProtocolVehicle;
//=============================================================================
class ProtocolVehicles : public ProtocolBase
{
    Q_OBJECT
public:
    ProtocolVehicles(QObject *parent = nullptr);

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
        int vclass;
    };

    ProtocolVehicle *local;
    ProtocolServiceFirmware *firmware;

private:
    QMap<quint16, ProtocolVehicle *> squawkMap;
    ProtocolVehicle *addVehicle(quint64 squawk, ProtocolVehicles::IdentData ident);
    void identRequest(quint16 squawk);
    void identAssign(quint16 squawk, const ProtocolVehicles::IdentData &ident);

public slots:
    bool unpack(QByteArray packet);
    void vehicleSendUplink(quint16 squawk, QByteArray packet);
    void sendHeartbeat();

signals:
    void vehicleIdentified(ProtocolVehicle *protocol);
    void identAssigned(ProtocolVehicle *v, const IdentData &ident);
};
//=============================================================================
#endif
