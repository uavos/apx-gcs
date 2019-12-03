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
#include "DatalinkRemotes.h"
#include "Datalink.h"
#include "DatalinkRemote.h"

#include <App/AppLog.h>

#include <tcp_ports.h>
//=============================================================================
DatalinkRemotes::DatalinkRemotes(Datalink *datalink)
    : Fact(datalink,
           "hosts",
           tr("Remote servers"),
           tr("Discovered remote hosts"),
           Group | FlatModel,
           "download-network")
    , datalink(datalink)
    , m_connectedCount(0)
{
    //discover service
    f_discover = new Fact(this,
                          "discover",
                          tr("Discover"),
                          tr("Find remote servers"),
                          Bool | PersistentValue);
    f_discover->setDefaultValue(true);
    connect(f_discover, &Fact::valueChanged, this, &DatalinkRemotes::discover);
    udpDiscover = new QUdpSocket(this);
    connect(udpDiscover,
            &QUdpSocket::readyRead,
            this,
            &DatalinkRemotes::discoverRead,
            Qt::QueuedConnection);

    //connect to specific host menu
    f_add = new Fact(this, "add", tr("Connect to host"), tr("Create new connection"), Group);
    f_add->setIcon("plus-network");
    f_url = new Fact(f_add,
                     "host",
                     tr("Host address"),
                     tr("IP address of remote server"),
                     Text | PersistentValue);
    f_connect = new Fact(f_add, "connect", tr("Connect"), "", Action | Apply | CloseOnTrigger);
    connect(f_connect, &Fact::triggered, this, &DatalinkRemotes::connectTriggered);

    f_servers = new Fact(this, "servers", tr("Servers"), tr("Found servers"), Section | Const);
    connect(f_servers, &Fact::sizeChanged, this, &DatalinkRemotes::updateStatus);

    f_alloff = new Fact(this,
                        "alloff",
                        tr("Disconnect all"),
                        tr("Close all connections to remote servers"),
                        Action,
                        "lan-disconnect");

    discover();
    updateStatus();
}
//=============================================================================
DatalinkRemote *DatalinkRemotes::registerHost(QUrl url)
{
    DatalinkRemote *c = remoteByAddr(QHostAddress(url.host()));
    if (c)
        return c;
    c = new DatalinkRemote(f_servers, datalink, url);
    connect(c, &DatalinkRemote::activeChanged, this, &DatalinkRemotes::updateStatus);
    connect(f_alloff, &Fact::triggered, c, [c]() { c->setValue(false); });
    apxMsg() << QString("#%1: %2").arg(tr("found server")).arg(url.toString());
    datalink->addConnection(c);
    return c;
}
DatalinkRemote *DatalinkRemotes::remoteByAddr(QHostAddress addr)
{
    if (addr.isNull())
        return nullptr;
    for (int i = 0; i < f_servers->size(); ++i) {
        DatalinkRemote *c = static_cast<DatalinkRemote *>(f_servers->child(i));
        if (!c->hostAddress.isEqual(addr))
            continue;
        return c;
    }
    return nullptr;
}
//=============================================================================
//=============================================================================
void DatalinkRemotes::discover(void)
{
    if (f_discover->value().toBool()) {
        if (udpDiscover->state() == QAbstractSocket::BoundState)
            return;
        if (udpDiscover->bind(QHostAddress::Any, UDP_PORT_DISCOVER)) {
            qDebug() << "binded";
            return;
        }
        QTimer::singleShot(5000, this, &DatalinkRemotes::discover);
    } else {
        if (udpDiscover->state() != QAbstractSocket::UnconnectedState) {
            udpDiscover->close();
            qDebug() << "closed";
        }
    }
    //apxConsoleW()<<"udp discover bind retry"<<udpReader->errorString();
}
void DatalinkRemotes::discoverRead(void)
{
    const QByteArray tail("@server.gcs.uavos.com");
    while (udpDiscover->hasPendingDatagrams()) {
        qint64 sz = udpDiscover->pendingDatagramSize();
        if (sz < tail.size() || sz > (tail.size() * 2)) {
            udpDiscover->receiveDatagram(0);
            continue;
        }
        QNetworkDatagram datagram = udpDiscover->receiveDatagram(255);
        if (!datagram.isValid())
            continue;
        //apxConsole()<<datagram;
        QByteArray data = datagram.data();
        if (!data.endsWith(tail))
            continue;
        if (datalink->f_server->active() && DatalinkTcpSocket::isLocalHost(datagram.senderAddress()))
            continue;
        QString uname = QString(data.left(data.indexOf('@')));
        //register or update host
        QUrl url;
        url.setScheme("tcp");
        url.setUserName(uname);
        QString saddr = datagram.senderAddress().toString();
        url.setHost(saddr.mid(saddr.lastIndexOf(':') + 1));
        DatalinkRemote *c = registerHost(url);
        c->updateTimeout();
    }
}
//=============================================================================
//=============================================================================
void DatalinkRemotes::updateStatus()
{
    //count connected
    int cnt = 0;
    for (int i = 0; i < f_servers->size(); ++i) {
        Fact *f = f_servers->child(i);
        if (f->active())
            cnt++;
    }
    setConnectedCount(cnt);
    f_alloff->setEnabled(cnt > 0);
    setActive(cnt > 0);

    if (f_servers->size() > 0)
        setStatus(QString("%1/%2").arg(cnt).arg(f_servers->size()));
    else
        setStatus(QString());
}
//=============================================================================
void DatalinkRemotes::connectTriggered()
{
    QUrl url = DatalinkRemote::fixUrl(f_url->text());
    QHostAddress addr = QHostAddress(url.host());
    if (addr.isNull() || addr.isLoopback())
        return;
    DatalinkRemote *c = registerHost(url);
    c->setValue(true);
}
//=============================================================================
int DatalinkRemotes::connectedCount() const
{
    return m_connectedCount;
}
void DatalinkRemotes::setConnectedCount(int v)
{
    if (m_connectedCount == v)
        return;
    m_connectedCount = v;
    emit connectedCountChanged();
}
//=============================================================================
