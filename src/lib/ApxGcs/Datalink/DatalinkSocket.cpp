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
#include "DatalinkSocket.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <crc.h>

DatalinkSocket::DatalinkSocket(Fact *parent,
                               QAbstractSocket *_socket,
                               QHostAddress hostAddress,
                               quint16 hostPort,
                               quint16 rxNetwork,
                               quint16 txNetwork)
    : DatalinkConnection(parent,
                         "socket#",
                         hostAddress.toString().append(":%1").arg(hostPort),
                         "",
                         rxNetwork,
                         txNetwork)
    , _socket(_socket)
    , _hostAddress(hostAddress)
    , _hostPort(hostPort)
{
    setEncoder(&_enc);
    setDecoder(&_dec);

    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(_socket,
            &QAbstractSocket::disconnected,
            this,
            &DatalinkSocket::socketDisconnected,
            Qt::QueuedConnection);

    connect(_socket,
            static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::errorOccurred),
            this,
            &DatalinkSocket::socketError);

    connect(_socket, &QAbstractSocket::stateChanged, this, &DatalinkSocket::socketStateChanged);
}

void DatalinkSocket::setRemoteUrl(QUrl url)
{
    // qDebug() << url << url.isValid() << url.toString();
    setUrl(url.toString());
    _hostAddress = QHostAddress(url.host());
    _hostPort = static_cast<quint16>(url.port());
}

bool DatalinkSocket::isLocalHost(const QHostAddress address)
{
    if (address.isLoopback())
        return true;
    for (const auto &i : QNetworkInterface::allAddresses())
        if (address.isEqual(i))
            return true;
    return false;
}
bool DatalinkSocket::isEqual(const QHostAddress address)
{
    return _hostAddress.isEqual(address);
}

void DatalinkSocket::socketDisconnected()
{
    resetDataStream();

    closed();
    emit disconnected();
}

void DatalinkSocket::socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    apxMsg() << QString("#%1 (%2:%3)")
                    .arg(_socket->errorString())
                    .arg(_socket->peerAddress().toString())
                    .arg(_socket->peerPort());
    setStatus("Error");
    emit error();

    close();
}

void DatalinkSocket::socketStateChanged(QAbstractSocket::SocketState socketState)
{
    QString s;
    switch (socketState) {
    //default:
    case QAbstractSocket::UnconnectedState:
        break;
    case QAbstractSocket::HostLookupState:
        s = "Lookup";
        break;
    case QAbstractSocket::ConnectingState:
        s = "Connecting";
        break;
    case QAbstractSocket::ConnectedState:
        s = "Connected";
        break;
    case QAbstractSocket::BoundState:
        s = "Bound";
        break;
    case QAbstractSocket::ClosingState:
        s = "Closing";
        break;
    case QAbstractSocket::ListeningState:
        s = "Listening";
        break;
    }
    setStatus(s);
}

void DatalinkSocket::close()
{
    _socket->abort();
    closed();
}
