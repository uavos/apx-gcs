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
#include "DatalinkRemote.h"
#include "Datalink.h"
#include "DatalinkRemotes.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <tcp_ports.h>

DatalinkRemote::DatalinkRemote(Fact *parent, Datalink *datalink, QUrl url)
    : DatalinkTcp(parent,
                  new QTcpSocket(),
                  Datalink::SERVERS | Datalink::LOCAL,
                  Datalink::SERVERS | Datalink::CLIENTS | Datalink::LOCAL)
    , datalink(datalink)
    , retry(0)
{
    setRemoteUrl(url);

    updateStatsTimer.setSingleShot(true);
    connect(&updateStatsTimer, &QTimer::timeout, this, &DatalinkRemote::updateStats);

    connect(this, &DatalinkConnection::activatedChanged, this, [this]() {
        if (activated()) {
            retry = 0;
            open();
        } else {
            close();
        }
    });

    connect(this, &DatalinkTcp::disconnected, this, &DatalinkRemote::reconnect);
    connect(this, &DatalinkTcp::error, this, &DatalinkRemote::reconnect);

    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout, this, &DatalinkRemote::open);

    updateStats();
}

void DatalinkRemote::setRemoteUrl(QUrl url)
{
    qDebug() << url << url.isValid() << url.toString();
    url = fixUrl(url);
    setUrl(url.toString());
    _hostAddress = QHostAddress(url.host());
    _hostPort = static_cast<quint16>(url.port());
}
QUrl DatalinkRemote::fixUrl(QUrl url)
{
    if (url.scheme().isEmpty()) {
        QString s = url.toString();
        url.setUrl(QString("%1://%2").arg("http").arg(s));
    }
    if (url.port() <= 0) {
        quint16 v = TCP_PORT_SERVER;
        url.setPort(v);
    }
    return url;
}

void DatalinkRemote::updateStats()
{
    if (time.isValid()) {
        int t = time.elapsed() / 1000;
        setDescr(QString("%1 (%2)")
                     .arg(t >= 60 ? tr("No service") : tr("Alive"))
                     .arg(t == 0    ? tr("now")
                          : t >= 60 ? QString("%1 %2").arg(t / 60).arg(tr("min"))
                                    : QString("%1 %2").arg(t).arg(tr("sec"))));
        updateStatsTimer.start(t > 60 ? 60000 : 5000);
    }
}

void DatalinkRemote::updateTimeout()
{
    time.start();
    updateStatsTimer.start(1000);
}

void DatalinkRemote::reconnect()
{
    if (activated()) {
        setStatus(QString("%1 %2").arg(tr("Retry")).arg(retry));
        reconnectTimer.start(1000 + (retry > 100 ? 100 : retry) * 200);
    } else {
        setStatus(QString());
    }
    apxMsg() << QString("#%1: %2").arg(tr("server disconnected")).arg(title());
}

void DatalinkRemote::open()
{
    //check if already present in connections
    for (int i = 0; i < datalink->connections.size(); ++i) {
        DatalinkTcp *c = qobject_cast<DatalinkTcp *>(datalink->connections.at(i));
        if (!c)
            continue;
        if (!c->active())
            continue;
        if (!c->isEqual(_hostAddress))
            continue;
        setActivated(false);
        apxMsgW() << tr("Connection exists");
        return;
    }

    //open
    retry++;
    connectToHost(_hostAddress, _hostPort);
}

void DatalinkRemote::hostLookupDone(QHostInfo info)
{
    if (info.error() != QHostInfo::NoError) {
        apxMsgW() << info.errorString();
        return;
    }
    qDebug() << "resolve:" << info.addresses();
    _hostAddress = info.addresses().first();
    open();
}
