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
#include <App/AppLog.h>

#include <Dictionary/MandalaIndex.h>
#include <Xbus/XbusPacket.h>
#include <Xbus/XbusVehicle.h>
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
    , m_blockControls(false)
    , m_blockService(false)
{
    connect(this, &Fact::valueChanged, this, [this]() { _allowed = value().toBool(); });

    connect(this, &DatalinkConnection::rxNetworkChanged, this, &DatalinkConnection::updateDescr);
    connect(this, &DatalinkConnection::txNetworkChanged, this, &DatalinkConnection::updateDescr);
    connect(this, &DatalinkConnection::blockControlsChanged, this, &DatalinkConnection::updateDescr);
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

    QString s = QString("RX: %1 / TX: %2").arg(rx.join(',')).arg(tx.join(','));
    if (m_blockControls)
        s.append(" NOCTR");
    setDescr(s);
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
    bool ctr = isControlPacket(packet);
    if (m_blockControls && ctr)
        return;
    if (m_blockService && (!ctr))
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
bool DatalinkConnection::isControlPacket(const QByteArray &packet) const
{
    uint16_t psize = static_cast<uint16_t>(packet.size());
    const uint8_t *pdata = reinterpret_cast<const uint8_t *>(packet.data());
    XbusStreamReader stream(pdata, psize);
    if (stream.tail() < sizeof(xbus::pid_t))
        return true;
    xbus::pid_t pid = stream.read<xbus::pid_t>();
    if (pid == mandala::idx_service || pid == mandala::idx_xpdr || pid == mandala::idx_ident)
        return false;

    if (pid != mandala::idx_dlink)
        return true;

    stream.read<xbus::vehicle::squawk_t>();
    pid = stream.read<xbus::pid_t>();
    return pid != mandala::idx_service;
}
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
bool DatalinkConnection::blockControls() const
{
    return m_blockControls;
}
void DatalinkConnection::setBlockControls(const bool &v)
{
    if (m_blockControls == v)
        return;
    m_blockControls = v;
    emit blockControlsChanged();
}
bool DatalinkConnection::blockService() const
{
    return m_blockService;
}
void DatalinkConnection::setBlockService(const bool &v)
{
    if (m_blockService == v)
        return;
    m_blockService = v;
    emit blockServiceChanged();
}
//=============================================================================
