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
