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
#include "DatalinkServer.h"
#include "Datalink.h"
#include "DatalinkTcpSocket.h"
#include "HttpService.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <tcp_ports.h>
//=============================================================================
DatalinkServer::DatalinkServer(Datalink *datalink)
    : Fact(datalink,
           "server",
           tr("Server"),
           tr("Remote clients connections"),
           Group | Bool | FlatModel,
           "lan-connect")
    , datalink(datalink)
    , announceString(QString("%1@server.gcs.uavos.com").arg(App::username()).toUtf8())
{
    f_listen = new Fact(this,
                        "listen",
                        tr("Listen"),
                        tr("Accept incoming connections"),
                        Bool | PersistentValue,
                        "access-point-network");
    f_listen->setDefaultValue(true);
    connect(f_listen, &Fact::valueChanged, this, [this]() { setValue(f_listen->value()); });
    setValue(f_listen->value());
    connect(this, &Fact::valueChanged, this, [this]() { f_listen->setValue(value()); });

    f_extctr = new Fact(this,
                        "extctr",
                        tr("Controls from clients"),
                        tr("Don't block uplink from remote clients"),
                        Bool | PersistentValue,
                        "remote");
    f_extctr->setDefaultValue(true);
    connect(f_extctr, &Fact::valueChanged, this, &DatalinkServer::updateClientsNetworkMode);

    f_extsrv = new Fact(this,
                        "extsrv",
                        tr("Service requests from clients"),
                        tr("Don't block service requests from clients"),
                        Bool | PersistentValue,
                        "puzzle");
    f_extsrv->setDefaultValue(true);
    connect(f_extsrv, &Fact::valueChanged, this, &DatalinkServer::updateClientsNetworkMode);

    f_clients = new Fact(this, "clients", tr("Clients"), tr("Connected clients"), Section | Const);
    connect(f_clients, &Fact::sizeChanged, this, &DatalinkServer::updateStatus);

    f_alloff = new Fact(this,
                        "alloff",
                        tr("Disconnect all"),
                        tr("Drop all client connections"),
                        Action,
                        "lan-disconnect");

    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &DatalinkServer::newConnection);

    //discovery announce
    udpAnnounce = new QUdpSocket(this);
    announceTimer.setInterval(2000);
    connect(&announceTimer, &QTimer::timeout, this, &DatalinkServer::announce);

    //http service
    http = new HttpService(this);

    connect(f_listen, &Fact::valueChanged, this, &DatalinkServer::serverActiveChanged);

    updateStatus();
    QTimer::singleShot(500, this, &DatalinkServer::serverActiveChanged);
}
//=============================================================================
void DatalinkServer::updateStatus()
{
    int cnt = f_clients->size();
    f_alloff->setEnabled(cnt > 0);
    setStatus(cnt > 0 ? QString::number(cnt) : "");
}
//=============================================================================
void DatalinkServer::serverActiveChanged()
{
    bool active = f_listen->value().toBool();
    if (!active) {
        f_alloff->trigger();
        tcpServer->close();
        setActive(false);
        announceTimer.stop();
        apxMsg() << tr("Datalink server disabled");
        return;
    }
    //activate server
    apxMsg() << tr("Datalink server enabled");
    retryBind = 0;
    tryBindServer();
}
//=============================================================================
void DatalinkServer::announce(void)
{
    if (active()) {
        udpAnnounce->writeDatagram(announceString, QHostAddress::Broadcast, UDP_PORT_DISCOVER);
        //qDebug()<<"announce";
    }
}
//=============================================================================
//=============================================================================
void DatalinkServer::tryBindServer()
{
    if (!f_listen->value().toBool())
        return;
    if (!tcpServer->listen(QHostAddress::Any, TCP_PORT_SERVER)) {
        setActive(false);
        //server port is busy by another local GCU
        if (++retryBind <= 1) {
            apxMsgW() << tr("Unable to start server").append(":") << tcpServer->errorString();
        }
        int to = 1000 + (retryBind / 10) * 1000;
        QTimer::singleShot(to > 10000 ? 10000 : to, this, SLOT(tryBindServer()));
        emit bindError();
        return;
    }
    setActive(true);
    if (retryBind)
        apxMsg() << tr("Server binded");
    retryBind = 0;
    emit binded();
    announceTimer.start();
}
//=============================================================================
void DatalinkServer::newConnection()
{
    while (tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = tcpServer->nextPendingConnection();
        if (!f_listen->value().toBool()) {
            socket->disconnectFromHost();
            continue;
        }
        //check loops
        /*DatalinkHost *host=f_datalink->f_hosts->hostByAddr(socket->peerAddress());
    if(host && host->active()){
      apxMsgW()<<tr("Client refused").append(":")<<socket->peerAddress().toString();
      socket->disconnectFromHost();
      continue;
    }*/
        //new DatalinkClient(this,socket);
        DatalinkTcpSocket *c = new DatalinkTcpSocket(f_clients, socket, 0, 0);
        updateClientsNetworkMode();
        c->setTitle(socket->peerAddress().toString());
        connect(f_alloff, &Fact::triggered, c, &DatalinkConnection::close);
        connect(c, &DatalinkTcpSocket::httpRequest, http, &HttpService::httpRequest);
        datalink->addConnection(c);
        c->setValue(true);
    }
}
//=============================================================================
void DatalinkServer::updateClientsNetworkMode()
{
    quint16 rxNetwork = Datalink::CLIENTS | Datalink::LOCAL;
    quint16 txNetwork = Datalink::CLIENTS | Datalink::SERVERS | Datalink::LOCAL;

    bool extctr = f_extctr->value().toBool();
    bool extsrv = f_extsrv->value().toBool();

    if (!(extctr || extsrv))
        rxNetwork = 0;

    for (int i = 0; i < f_clients->size(); ++i) {
        DatalinkConnection *c = static_cast<DatalinkConnection *>(f_clients->child(i));
        c->setRxNetwork(rxNetwork);
        c->setTxNetwork(txNetwork);
        c->setBlockControls(rxNetwork && (!extctr));
        c->setBlockService(rxNetwork && (!extsrv));
    }
}
//=============================================================================
//=============================================================================
