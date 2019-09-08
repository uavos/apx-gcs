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
#ifndef DatalinkServer_H
#define DatalinkServer_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
#include <QtNetwork>
class Datalink;
class HttpService;
//=============================================================================
class DatalinkServer : public Fact
{
    Q_OBJECT
public:
    explicit DatalinkServer(Datalink *datalink);

    Fact *f_listen;
    Fact *f_extctr;
    Fact *f_extsrv;

    Fact *f_clients;

    Fact *f_alloff;

private:
    Datalink *datalink;

    QTcpServer *tcpServer;
    uint retryBind;

    HttpService *http;

    QUdpSocket *udpAnnounce;
    QTimer announceTimer;
    QByteArray announceString;

private slots:
    void updateStatus();
    void updateClientsNetworkMode();

    void serverActiveChanged();
    void tryBindServer();

    void announce(void);

    //tcp server
    void newConnection();

signals:
    void bindError();
    void binded();
};
//=============================================================================
#endif
