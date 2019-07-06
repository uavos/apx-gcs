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
#include "DatalinkConnection.h"
#include "Datalink.h"
#include <ApxLog.h>
//=============================================================================
DatalinkConnection::DatalinkConnection(Fact *parent,
                                       const QString &name,
                                       const QString &title,
                                       const QString &descr,
                                       quint16 rxNetwork,
                                       quint16 txNetwork)
    : Fact(parent, name, title, descr, Bool)
    , _allowed(true)
    , m_rxNetwork(rxNetwork)
    , m_txNetwork(txNetwork)
{
    connect(this, &Fact::valueChanged, this, [this]() { _allowed = value().toBool(); });

    connect(this, &DatalinkConnection::rxNetworkChanged, this, &DatalinkConnection::updateDescr);
    connect(this, &DatalinkConnection::txNetworkChanged, this, &DatalinkConnection::updateDescr);
    updateDescr();
}
//=============================================================================
void DatalinkConnection::updateDescr()
{
    const QMetaEnum &e = QMetaEnum::fromType<Datalink::NetworkMask>();
    QStringList rx, tx;
    for (int i = 0; i < e.keyCount(); ++i) {
        if (m_rxNetwork & e.value(i))
            rx.append(e.key(i));
        if (m_txNetwork & e.value(i))
            tx.append(e.key(i));
    }
    if (rx.isEmpty())
        rx.append("NO");
    if (tx.isEmpty())
        tx.append("NO");
    setDescr(QString("RX: %1 / TX: %2").arg(rx.join(',')).arg(tx.join(',')));
}
//=============================================================================
void DatalinkConnection::sendPacket(QByteArray packet, quint16 network)
{
    if (!(_allowed && active()))
        return;
    if (!(m_txNetwork & network))
        return;
    write(packet);
}
//=============================================================================
void DatalinkConnection::readDataAvailable()
{
    QByteArray packet = read();
    if (packet.isEmpty())
        return;
    if (!(_allowed && active()))
        return;
    emit packetReceived(packet, m_rxNetwork);
}
void DatalinkConnection::opened()
{
    if (!value().toBool()) {
        QTimer::singleShot(0, this, &DatalinkConnection::close);
        return;
    }
    setActive(true);
}
void DatalinkConnection::closed()
{
    setActive(false);
}
//=============================================================================
//=============================================================================
void DatalinkConnection::open()
{
    qWarning() << "not implemented";
}
void DatalinkConnection::close()
{
    qWarning() << "not implemented";
}
QByteArray DatalinkConnection::read()
{
    qWarning() << "not implemented";
    return QByteArray();
}
void DatalinkConnection::write(const QByteArray &packet)
{
    Q_UNUSED(packet)
    qWarning() << "not implemented";
}
//=============================================================================
//=============================================================================
quint16 DatalinkConnection::txNetwork() const
{
    return m_txNetwork;
}
void DatalinkConnection::setTxNetwork(const quint16 &v)
{
    if (m_txNetwork == v)
        return;
    m_txNetwork = v;
    emit txNetworkChanged();
}
quint16 DatalinkConnection::rxNetwork() const
{
    return m_rxNetwork;
}
void DatalinkConnection::setRxNetwork(const quint16 &v)
{
    if (m_rxNetwork == v)
        return;
    m_rxNetwork = v;
    emit rxNetworkChanged();
}
//=============================================================================
