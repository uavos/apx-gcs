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
#include "DatalinkSocketTcp.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <XbusPacket.h>

DatalinkSocketTcp::DatalinkSocketTcp(Fact *parent, QUrl url)
    : DatalinkSocket(parent,
                     new QTcpSocket(),
                     QHostAddress(url.host()),
                     static_cast<quint16>(url.port()),
                     Datalink::SERVERS | Datalink::LOCAL,
                     Datalink::SERVERS | Datalink::CLIENTS | Datalink::LOCAL)
    , _tcp(qobject_cast<QTcpSocket *>(_socket))
{
    setRemoteUrl(url);

    _tcp->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(_tcp, &QTcpSocket::connected, this, [this]() {
        retry = 0;
        apxMsg() << QString("#%1: %2:%3")
                        .arg(tr("TCP connected"))
                        .arg(_tcp->peerAddress().toString())
                        .arg(_tcp->peerPort());
        opened();
    });
    connect(_tcp, &QTcpSocket::readyRead, this, &DatalinkSocketTcp::readDataAvailable);

    reconnectTimer.setSingleShot(true);
    connect(this, &DatalinkSocket::disconnected, this, &DatalinkSocketTcp::reconnect);
    connect(this, &DatalinkSocket::error, this, &DatalinkSocketTcp::reconnect);
    connect(&reconnectTimer, &QTimer::timeout, this, &DatalinkSocketTcp::open);

    connect(this, &DatalinkConnection::activatedChanged, this, [this]() {
        if (activated()) {
            retry = 0;
            open();
        } else {
            reconnectTimer.stop();
            close();
        }
    });
}

void DatalinkSocketTcp::open()
{
    if (!activated())
        return;

    setRemoteUrl(url()); // re-parse url in case it was edited

    retry++;
    if (_tcp->isOpen())
        _tcp->abort();
    _tcp->connectToHost(_hostAddress, _hostPort);
}

void DatalinkSocketTcp::reconnect()
{
    if (activated()) {
        setStatus(QString("%1 %2").arg(tr("Retry")).arg(retry));
        reconnectTimer.start(1000 + (retry > 100 ? 100 : retry) * 200);
    } else {
        setStatus(QString());
    }
}

QByteArray DatalinkSocketTcp::read()
{
    if (!_tcp->isOpen())
        return {};
    return _tcp->read(xbus::size_packet_max * 2);
}

void DatalinkSocketTcp::write(const QByteArray &packet)
{
    if (_tcp->state() != QAbstractSocket::ConnectedState)
        return;
    _tcp->write(packet);
}
