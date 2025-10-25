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
#include "DatalinkSerialRemoteTcp.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

DatalinkSerialRemoteTcp::DatalinkSerialRemoteTcp(Fact *parent, QString host, int port)
    : DatalinkConnection(parent,
                         "remote_serial#",
                         QString("[tcp] %1:%2").arg(host).arg(port),
                         "",
                         Datalink::CLIENTS | Datalink::LOCAL,
                         Datalink::CLIENTS | Datalink::LOCAL)
    , m_host(host)
    , m_port(port)
{
    f_state = new Fact(this, "state", tr("Unconnected"), "");
    f_remove = new Fact(this, "remove", tr("Remove"), "", Action | Remove | CloseOnTrigger);

    m_noDataTimer.setSingleShot(true);
    m_noDataTimer.setInterval(3000);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &DatalinkSerialRemoteTcp::onErrorOccured);
    connect(&m_socket, &QTcpSocket::stateChanged, this, &DatalinkSerialRemoteTcp::onStateChanged);
    connect(&m_socket, &QTcpSocket::readyRead, this, &DatalinkSerialRemoteTcp::readDataAvailable);
    connect(&m_socket, &QTcpSocket::readyRead, &m_noDataTimer, qOverload<>(&QTimer::start));
    connect(&m_noDataTimer, &QTimer::timeout, this, &DatalinkSerialRemoteTcp::onNoDataTimerTimeout);
    connect(this,
            &DatalinkConnection::activatedChanged,
            this,
            &DatalinkSerialRemoteTcp::onActivatedChanged);
    connect(f_remove, &Fact::triggered, this, &DatalinkSerialRemoteTcp::onRemoveTriggered);

    setEncoder(&m_enc);
    setDecoder(&m_dec);
}

QString DatalinkSerialRemoteTcp::getHost() const
{
    return m_host;
}

int DatalinkSerialRemoteTcp::getPort() const
{
    return m_port;
}

void DatalinkSerialRemoteTcp::onErrorOccured(QAbstractSocket::SocketError socketError)
{
    apxConsoleW() << "TCP error:" << m_socket.errorString();

    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        m_socket.abort();
        apxMsgW() << tr("TCP disconnected: %1").arg(m_host);
    }
}

void DatalinkSerialRemoteTcp::onStateChanged(QAbstractSocket::SocketState socketState)
{
    QMap<QAbstractSocket::SocketState, QString> map
        = {{QAbstractSocket::UnconnectedState, "Unconnected"},
           {QAbstractSocket::HostLookupState, "Host lookup"},
           {QAbstractSocket::ConnectingState, "Connecting"},
           {QAbstractSocket::ConnectedState, "Connected"},
           {QAbstractSocket::BoundState, "Bound"},
           {QAbstractSocket::ClosingState, "Closing"}};
    auto stateString = map.value(socketState, "Unknown state");
    setDescr(stateString);
    f_state->setTitle(stateString);
    if (socketState == QAbstractSocket::ConnectedState) {
        setActive(true);
    } else {
        setActive(false);
    }
}

void DatalinkSerialRemoteTcp::onActivatedChanged()
{
    if (activated()) {
        open();
    } else {
        close();
    }
}

void DatalinkSerialRemoteTcp::onNoDataTimerTimeout()
{
    close();
    if (activated()) {
        open();
    }
}

void DatalinkSerialRemoteTcp::onRemoveTriggered()
{
    close();
    setParentFact(nullptr);
    deleteLater();
}

void DatalinkSerialRemoteTcp::open()
{
    if (!activated())
        return;

    m_socket.connectToHost(m_host, m_port);
    m_noDataTimer.start();
}

void DatalinkSerialRemoteTcp::close()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        apxMsg() << tr("TCP connection closed: %1").arg(m_host);
    }
    m_socket.abort();
    closed();
}

void DatalinkSerialRemoteTcp::write(const QByteArray &packet)
{
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        auto bytesWritten = m_socket.write(packet);
        if (bytesWritten >= 0 && bytesWritten != packet.size()) {
            qWarning() << "TCP: Packet was not sent completely";
        }
    }
}

QByteArray DatalinkSerialRemoteTcp::read()
{
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        resetDataStream();
        return {};
    }

    return m_socket.readAll();
}
