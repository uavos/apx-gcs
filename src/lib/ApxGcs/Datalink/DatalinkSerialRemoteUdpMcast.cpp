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
#include "DatalinkSerialRemoteUdpMcast.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

DatalinkSerialRemoteUdpMcast::DatalinkSerialRemoteUdpMcast(Fact *parent, QString mcast, int port)
    : DatalinkConnection(parent,
                         "remote_serial_udp#",
                         QString("[udp_mcast] %1:%2").arg(mcast).arg(port),
                         "",
                         Datalink::CLIENTS | Datalink::LOCAL,
                         Datalink::CLIENTS | Datalink::LOCAL)
    , m_mcast(mcast)
    , m_port(port)
{
    f_state = new Fact(this, "state", tr("Host"), "");
    f_remove = new Fact(this, "remove", tr("Remove"), "", Action | Remove | CloseOnTrigger);

    f_state->setText("Not found");

    m_noDataTimer.setSingleShot(true);
    m_noDataTimer.setInterval(5000);
    connect(&m_readSocket,
            &QUdpSocket::errorOccurred,
            this,
            &DatalinkSerialRemoteUdpMcast::onErrorOccured);
    connect(&m_writeSocket,
            &QUdpSocket::errorOccurred,
            this,
            &DatalinkSerialRemoteUdpMcast::onErrorOccured);
    connect(&m_readSocket,
            &QUdpSocket::readyRead,
            this,
            &DatalinkSerialRemoteUdpMcast::readDataAvailable);
    connect(&m_readSocket, &QUdpSocket::readyRead, &m_noDataTimer, qOverload<>(&QTimer::start));
    connect(&m_noDataTimer,
            &QTimer::timeout,
            this,
            &DatalinkSerialRemoteUdpMcast::onNoDataTimerTimeout);
    connect(this,
            &DatalinkConnection::activatedChanged,
            this,
            &DatalinkSerialRemoteUdpMcast::onActivatedChanged);
    connect(f_remove, &Fact::triggered, this, &DatalinkSerialRemoteUdpMcast::onRemoveTriggered);

    setEncoder(&m_enc);
    setDecoder(&m_dec);
}

QString DatalinkSerialRemoteUdpMcast::getHost() const
{
    return m_mcast;
}

int DatalinkSerialRemoteUdpMcast::getPort() const
{
    return m_port;
}

void DatalinkSerialRemoteUdpMcast::onErrorOccured(QAbstractSocket::SocketError socketError)
{
    apxConsoleW() << "Read udp error:" << m_readSocket.errorString();
    apxConsoleW() << "Write udp error:" << m_writeSocket.errorString();
}

void DatalinkSerialRemoteUdpMcast::onActivatedChanged()
{
    if (activated()) {
        open();
        setActive(true);
        setDescr("Running");
    } else {
        close();
        setActive(false);
        setDescr("");
    }
}

void DatalinkSerialRemoteUdpMcast::onNoDataTimerTimeout()
{
    close();
    if (activated()) {
        open();
    }
}

void DatalinkSerialRemoteUdpMcast::onRemoveTriggered()
{
    close();
    setParentFact(nullptr);
    deleteLater();
}

void DatalinkSerialRemoteUdpMcast::open()
{
    if (!activated())
        return;

    resetDataStream();
    m_readSocket.bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress);
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : ifaces) {
        m_readSocket.joinMulticastGroup(QHostAddress(m_mcast), iface);
    }
    m_writeSocket.bind();
    m_noDataTimer.start();
    setActive(true);
}

void DatalinkSerialRemoteUdpMcast::close()
{
    m_readSocket.close();
    m_writeSocket.close();
    closed();
}

void DatalinkSerialRemoteUdpMcast::write(const QByteArray &packet)
{
    if (!m_host.isEmpty()) {
        auto bytesWritten = m_writeSocket.writeDatagram(packet, QHostAddress(m_host), m_port);
        if (bytesWritten >= 0 && bytesWritten != packet.size()) {
            qWarning() << "UDP: Packet was not sent completely";
        }
    } else {
        qWarning() << "write: mcast streams not found";
    }
}

QByteArray DatalinkSerialRemoteUdpMcast::read()
{
    QByteArray result;
    while (m_readSocket.hasPendingDatagrams()) {
        auto datagram = m_readSocket.receiveDatagram();
        if (datagram.destinationAddress().toString() == m_mcast
            && datagram.destinationPort() == m_port) {
            if (m_host != datagram.senderAddress().toString()) {
                m_host = datagram.senderAddress().toString();
                f_state->setText(m_host);
                qDebug() << "mcast host found: " << m_host;
            }
            result.append(datagram.data());
        }
    }
    return result;
}
