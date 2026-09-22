#include "Swarm.h"

#include <tcp_ports.h>

Swarm::Swarm(Fact *parent)
    : Fact(parent, QString(PLUGIN_NAME).toLower(), tr("Swarm"), tr("Swarm management"), Group, "arch")
{
    f_swarm_enabled = new Fact(this, "enable", tr("Enable"), tr("Enable Swarm"), Fact::Bool, "link");
    f_swarm_master = new Fact(this,
                              "callsign",
                              tr("Unit Callsign"),
                              tr("Unit callsign of the swarm master"),
                              Text | PersistentValue);
    f_swarm_master->setDefaultValue("M-97");

    _swarm_timer.setInterval(500);
    connect(&_swarm_timer, &QTimer::timeout, this, &Swarm::onSwarmTimer);
    _swarm_timer.start();

    // UDP relay
    QString sect = tr("UDP relay");

    f_udp_enabled
        = new Fact(this,
                   "udp_enable",
                   tr("Enable UDP relay"),
                   tr("Send swarm bundles of local apx.sim to apx.sim on other computers"),
                   Fact::Bool,
                   "ip-network");
    f_udp_enabled->setSection(sect);

    f_udp_local
        = new Fact(this,
                   "udp_local",
                   tr("Local sim port"),
                   tr("UDP port to receive bus stream from local apx.sim (VCP udp, BUS mode)"),
                   Int | PersistentValue);
    f_udp_local->setSection(sect);
    f_udp_local->setMin(1);
    f_udp_local->setMax(65535);
    f_udp_local->setDefaultValue(9236); // apx.sim VCP udp default destination port

    f_udp_sims = new Fact(this,
                          "udp_sims",
                          tr("Remote sims"),
                          tr("apx.sim uplink on other computers: IP[:PORT], comma separated"),
                          Text | PersistentValue);
    f_udp_sims->setSection(sect);

    connect(f_udp_enabled, &Fact::valueChanged, this, &Swarm::udpBind);
    connect(f_udp_local, &Fact::valueChanged, this, [this]() {
        if (f_udp_enabled->value().toBool())
            udpBind();
    });
    connect(f_udp_sims, &Fact::valueChanged, this, &Swarm::udpUpdateSims);
    connect(&_udp_local, &QUdpSocket::readyRead, this, &Swarm::udpReadLocal);

    udpUpdateSims();
    _time.start();
}

void Swarm::udpBind()
{
    const bool was_bound = _udp_local.state() == QAbstractSocket::BoundState;
    _udp_local.close();
    _dec_local.reset();
    _sent.clear();

    if (!f_udp_enabled->value().toBool()) {
        if (was_bound)
            apxMsg() << "Swarm UDP" << tr("relay closed");
        return;
    }

    const auto port = f_udp_local->value().toUInt();
    if (!_udp_local.bind(port)) {
        apxMsgW() << "Swarm UDP" << tr("failed to bind port") << port;
        QTimer::singleShot(0, this, [this]() { f_udp_enabled->setValue(false); });
        return;
    }

    QStringList sims;
    for (const auto &p : _sims)
        sims << QString("%1:%2").arg(p.first.toString()).arg(p.second);
    apxMsg() << "Swarm UDP" << port << tr("to") << sims.join(", ");
}

void Swarm::udpUpdateSims()
{
    _sims.clear();

    const auto local_addrs = QNetworkInterface::allAddresses();

    const auto list = f_udp_sims->value().toString().split(QRegularExpression("[,;\\s]+"),
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
            apxMsgW() << "Swarm UDP" << tr("invalid address") << s;
            continue;
        }

        // local apx.sim is the source of swarm bundles
        if (port == UDP_PORT_AP_UPLINK && (addr.isLoopback() || local_addrs.contains(addr))) {
            apxMsgW() << "Swarm UDP" << tr("address points to local apx.sim") << s;
            continue;
        }

        _sims.append({addr, static_cast<quint16>(port)});
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

void Swarm::udpReadLocal()
{
    while (_udp_local.hasPendingDatagrams()) {
        const auto datagram = _udp_local.receiveDatagram();
        if (!datagram.isValid())
            continue;

        if (!f_udp_enabled->value().toBool())
            continue;

        // sim bus is COBS stream
        cobs_decode(_dec_local, datagram.data(), [this](const uint8_t *data, size_t size) {
            sendToSims(QByteArray(reinterpret_cast<const char *>(data), static_cast<int>(size)));
        });
    }
}

void Swarm::sendToSims(const QByteArray &packet)
{
    // raw xbus packet (pid + payload) exactly as decoded from local sim bus stream,
    // COBS and crc16 are stripped by decoder; swarm bundles only
    if (!is_swarm_bundle(packet))
        return;

    if (_sims.isEmpty())
        return;

    // remote sim retransmits received bundle to its bus (VCP BUS mode) and when
    // the relay is enabled there too, the packet comes back - send each packet once
    if (wasSent(packet))
        return;
    _sent.append({_time.elapsed(), packet});

    qDebug() << "TX" << packet.toHex();

    // apx.sim uplink port expects COBS stream with crc16, the same as from GCU
    const auto cnt = _enc.encode(packet.constData(), static_cast<size_t>(packet.size()));
    const QByteArray frame(reinterpret_cast<const char *>(_enc.data()), static_cast<int>(cnt));

    for (const auto &p : _sims)
        _udp_local.writeDatagram(frame, p.first, p.second);
}

void Swarm::dropExpired()
{
    // master publishes every 200ms and pid.seq (2 bits) repeats in 800ms,
    // so the same packet bytes within the window are loops only
    const auto t = _time.elapsed();
    while (!_sent.isEmpty() && t - _sent.first().first > 500)
        _sent.removeFirst();
}

bool Swarm::wasSent(const QByteArray &packet)
{
    dropExpired();
    for (const auto &i : _sent) {
        if (i.second == packet)
            return true;
    }
    return false;
}

void Swarm::onSwarmTimer()
{
    if (!f_swarm_enabled->value().toBool()) {
        return;
    }

    for (auto i : Fleet::instance()->facts()) {
        Unit *m_unit = static_cast<Unit *>(i);

        if (m_unit && m_unit->isIdentified()) {
            if (m_unit->title() == f_swarm_master->value().toString()) {
                //Tx data;
                //ID
                //SwarmPub::_id.src = conf::data.shiva.swarm.id;
                //SwarmPub::_id.type = mandala::bundle::swarm_data;
                //SwarmPub::_id.ext = 0;

                //Data
                //SwarmPub::_data.cmd_airspeed = m.cmd_airspeed;
                //SwarmPub::_data.cmd_altitude = m.cmd_altitude;
                //SwarmPub::_data.cmd_bearing = m.cmd_bearing;
                //SwarmPub::_data.cmd_lacc = RegHdg::lateral_acc();

                //SwarmPub::_data.bearing = m.bearing;

                //SwarmPub::_data.cmd_llh[0] = m_lat;
                //SwarmPub::_data.cmd_llh[1] = m_lon;
                //SwarmPub::_data.cmd_llh[2] = m_hmsl;

                //swarm id
                _swarm_data.id.src = 0;
                _swarm_data.id.type = mandala::bundle::swarm_data;
                _swarm_data.id.ext = 0;

                Mandala *mandala = m_unit->f_mandala;

                if (!mandala) {
                    break;
                }

                //swarm data
                _swarm_data.cmd_airspeed
                    = mandala->fact(mandala::cmd::nav::pos::airspeed::uid)->value().toFloat();

                _swarm_data.cmd_altitude
                    = mandala->fact(mandala::cmd::nav::pos::altitude::uid)->value().toFloat();

                _swarm_data.cmd_bearing = qDegreesToRadians(
                    mandala->fact(mandala::cmd::nav::pos::bearing::uid)->value().toFloat());

                _swarm_data.cmd_lacc
                    = mandala->fact(mandala::est::env::usrf::f15::uid)->value().toFloat();

                _swarm_data.bearing = qDegreesToRadians(
                    mandala->fact(mandala::est::nav::pos::bearing::uid)->value().toFloat());

                _swarm_data.cmd_llh[0] = mandala::deg_to_a32(
                    mandala->fact(mandala::est::nav::pos::lat::uid)->value().toDouble());

                _swarm_data.cmd_llh[1] = mandala::deg_to_a32(
                    mandala->fact(mandala::est::nav::pos::lon::uid)->value().toDouble());

                _swarm_data.cmd_llh[2]
                    = mandala->fact(mandala::est::nav::pos::hmsl::uid)->value().toFloat();

                swarm_update = true;
                break;
            }
        }
    }

    for (auto i : Fleet::instance()->facts()) {
        Unit *s_unit = static_cast<Unit *>(i);

        if (s_unit && s_unit->isIdentified()) {
            if (s_unit->title() == f_swarm_master->value().toString()) {
                continue;
            }

            if (swarm_update) {
                PData *pdata = s_unit->protocol()->data();
                if (pdata) {
                    QByteArray ba;
                    ba.resize(sizeof(swarm::swarm_s));
                    memcpy(ba.data(), &_swarm_data, sizeof(_swarm_data));

                    QVariant value = ba;
                    pdata->sendValue(mandala::cmd::nav::swarm::uid, value);
                }
            }
        }
    }

    swarm_update = false;
}
