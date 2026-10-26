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
#include "SimSwarm.h"
#include "Simulator.h"

#include <App/AppLog.h>

#include <tcp_ports.h>

#include <MandalaBundles.h>
#include <MandalaMetaTree.h>

SimSwarm::SimSwarm(Simulator *parent)
    : Fact(parent, "swarm", tr("Swarm"), tr("Relay swarm bundles"), Group)
    , _sim(parent)
{
    QString sect = tr("Swarm");

    f_enb = new Fact(this,
                     "enb",
                     tr("Enable Swarm"),
                     tr("Relay bundles of the master instance"),
                     Bool);
    f_enb->setSection(sect);

    f_port = new Fact(this,
                      "port",
                      tr("Master bus port"),
                      tr("UDP port of the master instance bus"),
                      Int | PersistentValue);
    f_port->setSection(sect);
    f_port->setMin(1);
    f_port->setMax(65535);
    f_port->setDefaultValue(9236); // apx.sim VCP udp default destination port

    f_peers = new Fact(this,
                       "peers",
                       tr("Peers"),
                       tr("Sims on other computers: IP[:PORT]"),
                       Text | PersistentValue);
    f_peers->setSection(sect);

    connect(f_enb, &Fact::valueChanged, this, &SimSwarm::udpBind);
    connect(f_port, &Fact::valueChanged, this, [this]() {
        if (f_enb->value().toBool())
            udpBind();
    });
    connect(f_peers, &Fact::valueChanged, this, &SimSwarm::updatePeers);
    connect(&_udp, &QUdpSocket::readyRead, this, &SimSwarm::readMaster);

    updatePeers();
    _time.start();
}

void SimSwarm::udpBind()
{
    _udp.close();
    _dec.reset();
    _sent.clear();

    if (!f_enb->value().toBool())
        return;

    const auto port = f_port->value().toUInt();
    if (!_udp.bind(port)) {
        apxMsgW() << tr("Swarm") << tr("failed to bind UDP port") << port;
        QTimer::singleShot(0, this, [this]() { f_enb->setValue(false); });
        return;
    }

    qCDebug(SimLog) << "swarm relay on" << port;
}

void SimSwarm::updatePeers()
{
    _peers.clear();

    const auto list = f_peers->value().toString().split(QRegularExpression("[,;\\s]+"),
                                                        Qt::SkipEmptyParts);
    for (const auto &s : list) {
        QHostAddress addr(s);
        uint port = UDP_PORT_AP_UPLINK;
        bool ok = true;

        if (addr.isNull()) {
            const auto i = s.lastIndexOf(':');
            if (i > 0) {
                addr = QHostAddress(s.left(i));
                port = s.mid(i + 1).toUInt(&ok);
            }
        }

        if (addr.isNull() || !ok || port == 0 || port > 65535) {
            apxMsgW() << tr("Swarm") << tr("invalid peer address") << s;
            continue;
        }

        _peers.append({addr, static_cast<quint16>(port)});
    }
}

// decode COBS stream and call f(data, size) for each packet
template<typename F>
static void cobs_decode(CobsDecoder<> &dec, const QByteArray &data, F &&f)
{
    auto src = reinterpret_cast<const uint8_t *>(data.constData());
    auto cnt = static_cast<size_t>(data.size());
    while (cnt) {
        const auto dcnt = dec.decode(src, cnt);
        if (dec.status() == SerialDecoder::PacketAvailable)
            f(dec.data(), dec.size());
        if (!dcnt || dcnt >= cnt)
            break;
        src += dcnt;
        cnt -= dcnt;
    }
}

// (pid, bundle) packet of swarm bundle
static bool is_swarm_bundle(const QByteArray &packet)
{
    const auto size = static_cast<size_t>(packet.size());
    if (size < xbus::pid_s::psize() + sizeof(mandala::bundle::swarmid_s))
        return false;

    // COBS encoder has no overflow checks
    if (size > xbus::size_packet_max)
        return false;

    XbusStreamReader rx(packet.constData(), size);
    xbus::pid_s pid;
    pid.read(&rx);
    return pid.uid == mandala::cmd::nav::swarm::uid && pid.pri != xbus::pri_request;
}

void SimSwarm::readMaster()
{
    while (_udp.hasPendingDatagrams()) {
        const auto datagram = _udp.receiveDatagram();
        if (!datagram.isValid())
            continue;

        if (!f_enb->value().toBool())
            continue;

        // sim bus is COBS stream
        cobs_decode(_dec, datagram.data(), [this](const uint8_t *data, size_t size) {
            sendToSims(QByteArray(reinterpret_cast<const char *>(data), static_cast<int>(size)));
        });
    }
}

void SimSwarm::sendToSims(const QByteArray &packet)
{
    // raw xbus packet (pid + payload) exactly as decoded from the master bus stream,
    // COBS and crc16 are stripped by decoder; swarm bundles only
    if (!is_swarm_bundle(packet))
        return;

    // a sim retransmits received bundle to its bus (VCP BUS mode) and when the
    // master bus stream is merged with it, the packet comes back - send it once
    if (wasSent(packet))
        return;
    _sent.append({_time.elapsed(), packet});

    qCDebug(SimLog) << "swarm" << packet.toHex();

    // apx.sim uplink port expects COBS stream with crc16, the same as from GCU
    const auto cnt = _enc.encode(packet.constData(), static_cast<size_t>(packet.size()));
    const QByteArray frame(reinterpret_cast<const char *>(_enc.data()), static_cast<int>(cnt));

    // all local SIL instances: the master is configured as such and ignores
    // received bundles, so there is no need to know which instance it is
    const auto instances = _sim->f_instances->value().toInt();
    for (int i = 0; i < instances; ++i)
        _udp.writeDatagram(frame, QHostAddress::LocalHost, Simulator::port_uplink(i));

    // sims on other computers
    for (const auto &p : _peers)
        _udp.writeDatagram(frame, p.first, p.second);
}

void SimSwarm::dropExpired()
{
    // master publishes every 200ms and pid.seq (2 bits) repeats in 800ms,
    // so the same packet bytes within the window are loops only
    const auto t = _time.elapsed();
    while (!_sent.isEmpty() && t - _sent.first().first > 500)
        _sent.removeFirst();
}

bool SimSwarm::wasSent(const QByteArray &packet)
{
    dropExpired();
    for (const auto &i : _sent) {
        if (i.second == packet)
            return true;
    }
    return false;
}
