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
#include "DatalinkServer.h"
#include "Datalink.h"
#include "DatalinkTcp.h"
#include "DatalinkUdp.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <tcp_ports.h>

#include <QDesktopServices>

DatalinkServer::DatalinkServer(Datalink *datalink)
    : Fact(datalink,
           "server",
           tr("Server"),
           tr("Remote clients connections"),
           Group | FlatModel,
           "lan-connect")
    , datalink(datalink)
{
    QUrl url;
    url.setScheme("tcp");
    url.setUserName(App::username());
    url.setHost(App::hostname());
    url.setPort(TCP_PORT_SERVER);
    url.setPath("/");
    announceHttpString = QString("service:gcs:").append(url.toString()).toUtf8();

    qDebug() << "service:gcs:" + url.toString();

    f_http = new Fact(this,
                      "http",
                      tr("HTTP server"),
                      tr("Port").append(": ").append(QString::number(TCP_PORT_SERVER)),
                      Bool | PersistentValue,
                      "code-tags");
    f_http->setDefaultValue(true);
    connect(f_http, &Fact::triggered, this, []() {
        QDesktopServices::openUrl(QUrl(QString("http://127.0.0.1:%1/").arg(TCP_PORT_SERVER)));
    });

    f_udp = new Fact(this,
                     "udp",
                     tr("UDP datalink"),
                     tr("Port").append(": ").append(QString::number(UDP_PORT_GCS_TLM)),
                     Bool | PersistentValue,
                     "access-point-network");
    f_udp->setDefaultValue(true);

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

    f_clients = new Fact(this, "clients", tr("Clients"), tr("Connected clients"), Section | Count);
    connect(f_clients, &Fact::sizeChanged, this, &DatalinkServer::updateStatus);

    f_alloff = new Fact(this,
                        "alloff",
                        tr("Disconnect all"),
                        tr("Drop all client connections"),
                        Action,
                        "lan-disconnect");

    httpServer = new QTcpServer(this);
    connect(httpServer, &QTcpServer::newConnection, this, &DatalinkServer::newHttpConnection);

    udpServer = new QUdpSocket(this);
    connect(udpServer, &QUdpSocket::readyRead, this, &DatalinkServer::udpReadyRead);

    //discovery announce
    udpAnnounce = new QUdpSocket(this);
    announceTimer.setSingleShot(true);
    announceTimer.setInterval(5333);
    connect(&announceTimer, &QTimer::timeout, this, &DatalinkServer::announce);

    connect(f_http, &Fact::valueChanged, this, &DatalinkServer::httpActiveChanged);
    connect(f_udp, &Fact::valueChanged, this, &DatalinkServer::udpActiveChanged);

    updateStatus();
    QTimer::singleShot(500, this, &DatalinkServer::httpActiveChanged);
    QTimer::singleShot(500, this, &DatalinkServer::udpActiveChanged);
}

void DatalinkServer::updateStatus()
{
    int cnt = f_clients->size();
    f_alloff->setEnabled(cnt > 0);
    setValue(cnt > 0 ? QString::number(cnt) : "");
}

void DatalinkServer::httpActiveChanged()
{
    if (!f_http->value().toBool()) {
        f_alloff->trigger();
        httpServer->close();
        f_http->setActive(false);
        announceTimer.stop();
        apxMsg() << tr("HTTP server disabled");
        return;
    }
    //activate server
    apxMsg() << tr("HTTP server enabled");
    retryBindHttp = 0;
    tryBindHttpServer();
}
void DatalinkServer::udpActiveChanged()
{
    if (!f_udp->value().toBool()) {
        udpServer->close();
        f_udp->setActive(false);
        apxMsg() << tr("UDP datalink disabled");
        return;
    }
    //activate server
    apxMsg() << tr("UDP datalink enabled");
    retryBindUdp = 0;
    tryBindUdpServer();
}

void DatalinkServer::announce(void)
{
    if (f_http->active()) {
        udpAnnounce->writeDatagram(announceHttpString, QHostAddress::Broadcast, UDP_PORT_DISCOVER);
        // qDebug() << "announce";
        announceTimer.start();
    }
}

void DatalinkServer::tryBindHttpServer()
{
    if (!f_http->value().toBool())
        return;
    if (!httpServer->listen(QHostAddress::Any, TCP_PORT_SERVER)) {
        f_http->setActive(false);
        //server port is busy by another local GCU
        if (++retryBindHttp <= 1) {
            apxMsgW() << tr("Unable to start server").append(":") << httpServer->errorString();
        }
        int to = 1000 + (retryBindHttp / 10) * 1000;
        QTimer::singleShot(to > 10000 ? 10000 : to, this, &DatalinkServer::tryBindHttpServer);
        emit bindError();
        return;
    }
    f_http->setActive(true);
    if (retryBindHttp)
        apxMsg() << tr("HTTP Server binded");
    retryBindHttp = 0;
    emit binded();
    announceTimer.start(1000);
}
void DatalinkServer::tryBindUdpServer()
{
    if (!f_udp->value().toBool())
        return;
    if (!udpServer->bind(QHostAddress::AnyIPv4, UDP_PORT_GCS_TLM)) {
        f_udp->setActive(false);
        //server port is busy by another local GCU
        if (++retryBindUdp <= 1) {
            apxMsgW() << tr("Unable to start server").append(":") << udpServer->errorString();
        }
        int to = 1000 + (retryBindUdp / 10) * 1000;
        QTimer::singleShot(to > 10000 ? 10000 : to, this, &DatalinkServer::tryBindUdpServer);
        emit bindError();
        return;
    }
    f_udp->setActive(true);
    if (retryBindUdp)
        apxMsg() << tr("UDP Server binded");
    retryBindUdp = 0;
}

void DatalinkServer::newHttpConnection()
{
    while (httpServer->hasPendingConnections()) {
        QTcpSocket *socket = httpServer->nextPendingConnection();
        if (!(f_http->active() && f_http->value().toBool())) {
            socket->disconnectFromHost();
            continue;
        }
        DatalinkTcp *c = new DatalinkTcp(f_clients, socket, 0, 0);
        updateClientsNetworkMode();
        c->setTitle(socket->peerAddress().toString());
        connect(f_alloff, &Fact::triggered, c, &DatalinkConnection::close);
        connect(c, &DatalinkTcp::httpRequest, this, &DatalinkServer::httpRequest);
        datalink->addConnection(c);
        c->setActivated(true);
    }
}
void DatalinkServer::udpReadyRead()
{
    while (udpServer->hasPendingDatagrams()) {
        if (!(f_udp->active() && f_udp->value().toBool())) {
            udpServer->receiveDatagram(0);
            continue;
        }

        QNetworkDatagram datagram = udpServer->receiveDatagram();
        if (!datagram.isValid())
            continue;

        const auto data = datagram.data();

        /*for (auto i : f_clients->facts()) {
            auto c = static_cast<DatalinkUdp *>(i);
            if (c->hostAddress == datagram.senderAddress()) {
                c->updateTimeout();
                c->write(data);
                continue;
            }
        }*/

        // //register or update host
        // QUrl url(data.mid(hdr.size()), QUrl::StrictMode);
        // if (!url.isValid())
        //     continue;

        // QString saddr = datagram.senderAddress().toString();
        // url.setHost(saddr.mid(saddr.lastIndexOf(':') + 1));
        // DatalinkRemote *c = registerHost(url);
        // c->updateTimeout();
    }
}

void DatalinkServer::updateClientsNetworkMode()
{
    quint16 rxNetwork = Datalink::CLIENTS | Datalink::LOCAL;
    quint16 txNetwork = Datalink::CLIENTS | Datalink::SERVERS | Datalink::LOCAL;

    bool extctr = f_extctr->value().toBool();
    bool extsrv = f_extsrv->value().toBool();

    if (!(extctr || extsrv))
        rxNetwork = 0;

    for (int i = 0; i < f_clients->size(); ++i) {
        auto c = static_cast<DatalinkConnection *>(f_clients->child(i));
        c->setRxNetwork(rxNetwork);
        c->setTxNetwork(txNetwork);
        c->setBlockControls(rxNetwork && (!extctr));
        c->setBlockService(rxNetwork && (!extsrv));
    }
}
