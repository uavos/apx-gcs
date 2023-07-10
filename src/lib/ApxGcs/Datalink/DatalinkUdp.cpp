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
#include "DatalinkUdp.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <crc.h>

DatalinkUdp::DatalinkUdp(Fact *parent,
                         QUdpSocket *socket,
                         QHostAddress hostAddress,
                         quint16 hostPort,
                         quint16 rxNetwork,
                         quint16 txNetwork)
    : DatalinkSocket(parent, socket, hostAddress, hostPort, rxNetwork, txNetwork)
    , _udp(socket)
{
    setActive(true);
}

void DatalinkUdp::socketDisconnected()
{
    DatalinkSocket::socketDisconnected();

    disconnect(_udp, nullptr, this, nullptr);
    _udp->deleteLater();
    deleteFact();
}

void DatalinkUdp::readDatagram(QNetworkDatagram datagram)
{
    _read_datagram = datagram;
    readDataAvailable();
}

QByteArray DatalinkUdp::read()
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

void DatalinkUdp::write(const QByteArray &packet)
{
    _udp->writeDatagram(packet, _hostAddress, _hostPort);
}
