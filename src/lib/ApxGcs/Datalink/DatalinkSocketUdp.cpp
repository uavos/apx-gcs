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
#include "DatalinkSocketUdp.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <crc.h>

DatalinkSocketUdp::DatalinkSocketUdp(Fact *parent,
                                     QUdpSocket *socket,
                                     QHostAddress hostAddress,
                                     quint16 hostPort,
                                     quint16 rxNetwork,
                                     quint16 txNetwork)
    : DatalinkSocket(parent, socket, hostAddress, hostPort, rxNetwork, txNetwork)
    , _udp(socket)
{
    setActive(true);
    setStatus("UDP");

    connect(this, &Fact::activeChanged, this, [this]() {
        if (!active())
            socketDisconnected();
    });
}

DatalinkSocketUdp::DatalinkSocketUdp(Fact *parent, QUrl url)
    : DatalinkSocket(parent,
                     new QUdpSocket(),
                     QHostAddress(url.host()),
                     static_cast<quint16>(url.port()),
                     Datalink::SERVERS | Datalink::LOCAL,
                     Datalink::SERVERS | Datalink::CLIENTS | Datalink::LOCAL)
    , _udp(qobject_cast<QUdpSocket *>(_socket))
{
    setRemoteUrl(url);

    connect(_udp, &QUdpSocket::readyRead, this, [this]() {
        while (_udp->hasPendingDatagrams()) {
            readDatagram(_udp->receiveDatagram());
        }
    });

    connect(this, &DatalinkConnection::activatedChanged, this, [this]() {
        if (activated()) {
            open();
        } else {
            close();
        }
    });
}

void DatalinkSocketUdp::open()
{
    if (_udp->isOpen())
        _udp->abort();

    const quint16 bindPort = _hostPort + 1;

    bool res = _udp->bind(QHostAddress::AnyIPv4,
                          bindPort,
                          QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (res) {
        setStatus("Listening");
        apxConsole() << "UDP socket bound to port" << bindPort;
    } else {
        setStatus("Bind error");
        apxConsoleW() << "Failed to bind UDP socket to port" << bindPort << ":"
                      << _udp->errorString();
    }
    setActive(res);
}

void DatalinkSocketUdp::socketDisconnected()
{
    DatalinkSocket::socketDisconnected();

    disconnect(_udp, nullptr, this, nullptr);
    deleteFact();
}

void DatalinkSocketUdp::readDatagram(QNetworkDatagram datagram)
{
    _read_datagram = datagram;
    readDataAvailable();
}

QByteArray DatalinkSocketUdp::read()
{
    if (!_read_datagram.isValid()) {
        // qDebug() << "invalid datagram";
        return {};
    }

    auto data = _read_datagram.data();
    _read_datagram = {};

    // qDebug() << data.toHex();

    return data;
}

void DatalinkSocketUdp::write(const QByteArray &packet)
{
    _udp->writeDatagram(packet, _hostAddress, _hostPort);
}
