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
#include "DatalinkConnection.h"
#include "Datalink.h"
#include <App/AppLog.h>

#include <Mandala/Mandala.h>

#include <XbusPacket.h>
#include <XbusUnit.h>

#include <Protocols/PStream.h>

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

void DatalinkConnection::setEncoder(SerialEncoder *encoder)
{
    _encoder = encoder;
}
void DatalinkConnection::setDecoder(SerialDecoder *decoder)
{
    _decoder = decoder;
    resetDataStream();
}
void DatalinkConnection::resetDataStream()
{
    if (_decoder)
        _decoder->reset();
    _rx_fifo.clear();
}

void DatalinkConnection::sendPacket(QByteArray packet, quint16 network)
{
    if (!(activated() && active()))
        return;
    if (!(m_txNetwork & network))
        return;

    if (!_encoder) {
        qDebug() << "TX no encoder";
        return;
    }

    auto cnt = _encoder->encode(packet.data(), static_cast<size_t>(packet.size()));
    if (cnt <= 0) {
        apxConsoleW() << "TX encode:" << packet.size() << cnt;
        return;
    }

    write(QByteArray((const char *) _encoder->data(), cnt));

    // qDebug() << "TX" << wcnt << cnt << packet.size()
    //          << QString::number(apx::crc32(packet.data(), packet.size()), 16);
}

void DatalinkConnection::readDataAvailable()
{
    for (;;) {
        QByteArray packet = _readPacket();
        if (packet.isEmpty())
            return;

        // qDebug() << "DPRX" << packet.size();

        if (!(activated() && active()))
            return;

        bool ctr = isControlPacket(packet);
        if (m_blockControls && ctr)
            return;
        if (m_blockService && (!ctr))
            return;

        emit packetReceived(packet, m_rxNetwork);
    }
}
QByteArray DatalinkConnection::_readPacket()
{
    // first return already decoded packets
    if (!_rx_fifo.empty()) {
        // qDebug() << "RX FIFO" << _rx_fifo.used();
        QTimer::singleShot(0, this, &DatalinkConnection::readDataAvailable);
        return _rx_fifo.dequeue();
    }

    // read PHY and continue decoding
    auto data = read();
    if (data.size() <= 0) {
        return {};
    }

    // schedule reads for next packet
    QTimer::singleShot(0, this, &DatalinkConnection::readDataAvailable);

    if (!_decoder) {
        qDebug() << "RX no decoder";
        return {};
    }

    // read bytes count (rcnt) might be more than one packet
    // decode all packets from raw data into fifo
    const uint8_t *src = reinterpret_cast<const uint8_t *>(data.constData());
    size_t cnt = data.size();
    bool pkt = false;
    while (cnt > 0) {
        // feed the decoder
        auto decoded_cnt = _decoder->decode(src, cnt);
        // qDebug() << "RX" << decoded_cnt << cnt;

        if (decoded_cnt <= 0)
            qWarning() << "RX decode:" << cnt << decoded_cnt;

        src += decoded_cnt;
        cnt -= decoded_cnt;

        // check if some pkt is available
        switch (_decoder->status()) {
        case SerialDecoder::PacketAvailable:
            pkt = true;
            // qDebug() << "RX PKT" << _decoder->size();
            _rx_fifo.enqueue(QByteArray((const char *) _decoder->data(), _decoder->size()));
            break;
        case SerialDecoder::DataAccepted:
        case SerialDecoder::DataDropped:
            break;
        default:
            apxConsoleW() << "RX err:" << _decoder->status() << data.size() << decoded_cnt;
            break;
        }
    }

    if (!pkt)
        return {};

    return _rx_fifo.dequeue();
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
    resetDataStream();
    setActive(false);
}

bool DatalinkConnection::isControlPacket(const QByteArray &packet) const
{
    PStreamReader stream(packet);

    if (stream.available() < xbus::pid_s::psize())
        return true;

    xbus::pid_s pid;
    pid.read(&stream);

    auto uid = pid.uid;

    if (mandala::cmd::env::nmt::match(uid))
        return false;
    if (mandala::cmd::env::unit::ident::match(uid))
        return false;
    if (mandala::cmd::env::unit::downlink::match(uid))
        return false;

    return true;
}

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
