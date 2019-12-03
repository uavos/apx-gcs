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
#include "DatalinkRemote.h"
#include "Datalink.h"
#include "DatalinkRemotes.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <tcp_ports.h>
//=============================================================================
DatalinkRemote::DatalinkRemote(Fact *parent, Datalink *datalink, QUrl url)
    : DatalinkTcpSocket(parent,
                        new QTcpSocket(),
                        Datalink::SERVERS | Datalink::LOCAL,
                        Datalink::SERVERS | Datalink::CLIENTS | Datalink::LOCAL)
    , datalink(datalink)
    , retry(0)
{
    setUrl(url);

    updateStatsTimer.setSingleShot(true);
    connect(&updateStatsTimer, &QTimer::timeout, this, &DatalinkRemote::updateStats);

    connect(this, &Fact::valueChanged, this, [this]() {
        if (value().toBool()) {
            retry = 0;
            open();
        } else {
            close();
        }
    });

    connect(this, &DatalinkTcpSocket::disconnected, this, &DatalinkRemote::reconnect);
    connect(this, &DatalinkTcpSocket::error, this, &DatalinkRemote::reconnect);

    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout, this, &DatalinkRemote::open);

    updateStats();
}
//=============================================================================
void DatalinkRemote::setUrl(QUrl url)
{
    //qDebug()<<url<<url.isValid()<<url.toString();
    url = fixUrl(url);
    //qDebug()<<url.scheme()<<url.host()<<url.port()<<url.authority()<<url.isValid();
    if (url.scheme() == "tcp" && url.port() == TCP_PORT_SERVER)
        setTitle(QString("%1@%2").arg(url.userInfo()).arg(url.host()));
    else
        setTitle(url.toString());
    hostAddress = QHostAddress(url.host());
    hostPort = static_cast<quint16>(url.port());
}
QUrl DatalinkRemote::fixUrl(QUrl url)
{
    if (url.scheme().isEmpty()) {
        QString s = url.toString();
        url.setUrl(QString("%1://%2").arg("tcp").arg(s));
    }
    if (url.port() <= 0) {
        quint16 v = TCP_PORT_SERVER;
        if (url.scheme().startsWith("http", Qt::CaseInsensitive))
            v = 80;
        url.setPort(v);
    }
    return url;
}
//=============================================================================
void DatalinkRemote::updateStats()
{
    if (time.isValid()) {
        int t = time.elapsed() / 1000;
        setDescr(QString("%1 (%2)")
                     .arg(t >= 60 ? tr("No service") : tr("Alive"))
                     .arg(t == 0 ? tr("now")
                                 : t >= 60 ? QString("%1 %2").arg(t / 60).arg(tr("min"))
                                           : QString("%1 %2").arg(t).arg(tr("sec"))));
        updateStatsTimer.start(t > 60 ? 60000 : 5000);
    }
}
//=============================================================================
void DatalinkRemote::updateTimeout()
{
    time.start();
    updateStatsTimer.start(1000);
}
//=============================================================================
void DatalinkRemote::reconnect()
{
    if (value().toBool()) {
        setStatus(QString("%1 %2").arg(tr("Retry")).arg(retry));
        reconnectTimer.start(1000 + (retry > 100 ? 100 : retry) * 200);
    } else {
        setStatus(QString());
    }
    apxMsg() << QString("#%1: %2").arg(tr("server disconnected")).arg(title());
}
//=============================================================================
void DatalinkRemote::open()
{
    //check if already present in connections
    for (int i = 0; i < datalink->connections.size(); ++i) {
        DatalinkTcpSocket *c = qobject_cast<DatalinkTcpSocket *>(datalink->connections.at(i));
        if (!c)
            continue;
        if (!c->active())
            continue;
        if (!c->hostAddress.isEqual(hostAddress))
            continue;
        setValue(false);
        apxMsgW() << tr("Connection exists");
        return;
    }

    //open
    retry++;
    connectToHost(hostAddress, hostPort);
}
//=============================================================================
void DatalinkRemote::hostLookupDone(QHostInfo info)
{
    if (info.error() != QHostInfo::NoError) {
        apxMsgW() << info.errorString();
        return;
    }
    qDebug() << "resolve:" << info.addresses();
    hostAddress = info.addresses().first();
    open();
}
//=============================================================================
