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
#ifndef Datalink_H
#define Datalink_H
//=============================================================================
#include "DatalinkConnection.h"
#include "DatalinkPorts.h"
#include "DatalinkRemotes.h"
#include "DatalinkServer.h"
#include "DatalinkStats.h"
#include <Fact/Fact.h>
#include <QtCore>
//=============================================================================
class Datalink : public Fact
{
    Q_OBJECT
    Q_ENUMS(NetworkMask)

    Q_PROPERTY(
        bool valid READ valid WRITE setValid NOTIFY validChanged) //true when any data ever received
    Q_PROPERTY(bool online READ online WRITE setOnline NOTIFY onlineChanged) //timeout received data
    Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY
                   errcntChanged) //global stream protocol errors

public:
    explicit Datalink(Fact *parent = nullptr);

    enum NetworkMask {
        LOCAL = 1,   //this GCS
        CLIENTS = 2, //remote client
        SERVERS = 4, //remote server
        PORTS = 8,   //local port
        AUX = 16,    //user port
    };
    Q_ENUM(NetworkMask)

    Fact *f_readonly;
    DatalinkServer *f_server;
    DatalinkRemotes *f_remotes;
    DatalinkPorts *f_ports;

    Fact *f_hbeat;

    DatalinkStats *f_stats;

    void addConnection(DatalinkConnection *c);
    QList<QPointer<DatalinkConnection>> connections;

private:
    QTimer heartbeatTimer; //data link alive for vehicle
    bool bReadOnly;
    QTimer onlineTimer;

private slots:
    void updateStatus();

    void readonlyChanged();
    void heartbeatTimeout();
    void hbeatChanged();

    //internal connections
private slots:
    void connectionPacketReceived(QByteArray packet, quint16 network);

    //counters
signals:
    void transmittedDataEvent(uint size);
    void receivedDataEvent(uint size);

    //external connections
public slots:
    void sendPacket(QByteArray packet);
signals:
    void packetReceived(QByteArray packet);
    void httpRequest(QTextStream &stream, QString req, bool *ok);
    void heartbeat();

    //-----------------------------------------
    //PROPERTIES
public:
    bool valid() const;
    void setValid(const bool &v);
    bool online() const;
    void setOnline(const bool &v);
    uint errcnt() const;
    void setErrcnt(const uint &v);

private:
    bool m_valid;
    bool m_online;
    uint m_errcnt;
signals:
    void validChanged();
    void onlineChanged();
    void errcntChanged();
};
//=============================================================================
#endif
