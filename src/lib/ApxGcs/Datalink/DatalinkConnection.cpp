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

#include <Mandala/Mandala.h>

#include <xbus/XbusPacket.h>
#include <xbus/XbusVehicle.h>
//=============================================================================
DatalinkConnection::DatalinkConnection(Fact *parent,
                                       const QString &name,
                                       const QString &title,
                                       const QString &descr,
                                       quint16 rxNetwork,
                                       quint16 txNetwork)
    : Fact(parent, name, title, descr, Bool)
    , m_activated(false)
    , m_rxNetwork(rxNetwork)
    , m_txNetwork(txNetwork)
    , m_blockControls(false)
    , m_blockService(false)
{
    connect(this, &Fact::valueChanged, this, [this]() { setActivated(value().toBool()); });
    connect(this, &DatalinkConnection::activatedChanged, this, [this]() { setValue(activated()); });

    connect(this, &DatalinkConnection::rxNetworkChanged, this, &DatalinkConnection::updateDescr);
    connect(this, &DatalinkConnection::txNetworkChanged, this, &DatalinkConnection::updateDescr);
    connect(this, &DatalinkConnection::blockControlsChanged, this, &DatalinkConnection::updateDescr);
    updateDescr();

    connect(this, &DatalinkConnection::urlChanged, this, &DatalinkConnection::updateTitle);
    connect(this, &DatalinkConnection::statusChanged, this, &DatalinkConnection::updateTitle);
    setUrl(title);
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
void DatalinkConnection::updateTitle()
{
    QString s = url();
    const QString &sx = status();
    if (s.isEmpty())
        s = sx;
    else if (!sx.isEmpty())
        s = QString("%1 [%2]").arg(s).arg(sx);
    setTitle(s);
}
//=============================================================================
void DatalinkConnection::sendPacket(QByteArray packet, quint16 network)
{
    if (!(activated() && active()))
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
    if (!(activated() && active()))
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
    if (!activated()) {
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
    if (stream.available() < xbus::pid_s::psize())
        return true;
    xbus::pid_s pid;
    pid.read(&stream);

    static constexpr const mandala::uid_t allowed[] = {
        mandala::cmd::env::nmt::uid,
        mandala::cmd::env::vehicle::ident::uid,
        mandala::cmd::env::vehicle::downlink::uid,
    };
    for (auto i : allowed)
        if (pid.uid == i)
            return false;

    return true;
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
QString DatalinkConnection::url() const
{
    return m_url;
}
void DatalinkConnection::setUrl(const QString &v)
{
    if (m_url == v)
        return;
    m_url = v;
    emit urlChanged();
}
QString DatalinkConnection::status() const
{
    return m_status;
}
void DatalinkConnection::setStatus(const QString &v)
{
    if (m_status == v)
        return;
    m_status = v;
    emit statusChanged();
}
bool DatalinkConnection::activated() const
{
    return m_activated;
}
void DatalinkConnection::setActivated(const bool &v)
{
    if (m_activated == v)
        return;
    m_activated = v;
    emit activatedChanged();
}
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
