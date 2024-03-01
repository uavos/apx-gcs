#include "UdpForwarding.h"

UdpForwarding::UdpForwarding(Fact *parent)
    : PortForwarding(parent,
                     "udp",
                     tr("UDP VCP forwarding"),
                     tr("Forward system UDP RX-TX ports to APX virtual port"),
                     "ip-network")
{
    f_udp_rx = new Fact(this,
                        "rx_port",
                        tr("RX UDP port"),
                        tr("UDP port to receive VCP data from"),
                        Int | PersistentValue);
    f_udp_rx->setMin(0);
    f_udp_rx->setMax(65535);
    f_udp_rx->setDefaultValue(9335);

    f_udp_tx = new Fact(this,
                        "tx_port",
                        tr("Destination"),
                        tr("UDP to send VCP data to [IP:]<PORT>"),
                        Text | PersistentValue);
    f_udp_tx->setDefaultValue("127.0.0.1:9333");
    connect(f_udp_tx, &Fact::valueChanged, this, &UdpForwarding::updateDest);
    updateDest();

    connect(this, &Fact::activeChanged, this, [this]() {
        if (!active()) {
            _udp.close();
            apxMsg() << "UDP" << f_udp_rx->value().toUInt() << tr("closed for")
                     << QString("VCP#%1").arg(f_vcpid->value().toInt());
        }
    });

    connect(f_enabled, &Fact::valueChanged, this, [this]() {
        if (f_enabled->value().toBool()) {
            if (!_udp.bind(f_udp_rx->value().toUInt())) {
                apxMsgW() << tr("Failed to bind UDP port") << f_udp_rx->value().toUInt();
                QTimer::singleShot(0, this, [this]() { setActive(false); });
            } else {
                setActive(true);
                apxMsg() << "UDP" << f_udp_rx->value().toUInt() << tr("bound to")
                         << QString("VCP#%1").arg(f_vcpid->value().toInt());
            }
        } else {
            qDebug() << "UDP" << f_udp_rx->value().toUInt() << tr("shutdown");
            setActive(false);
        }
    });

    connect(&_udp, &QUdpSocket::readyRead, this, &UdpForwarding::readyRead);
}

void UdpForwarding::updateDest()
{
    auto s = f_udp_tx->value().toString();
    if (s.contains(':')) {
        auto p = s.split(':');
        if (p.size() != 2)
            return;
        _dest_addr = QHostAddress(p.at(0));
        _dest_port = p.at(1).toUInt();

        if (_dest_addr.isNull())
            _dest_addr = QHostAddress::LocalHost;
    } else {
        _dest_addr = QHostAddress::LocalHost;
        _dest_port = s.toUInt();
    }
}

bool UdpForwarding::forwardToPHY(QByteArray data)
{
    if (!_dest_port)
        return false;

    if (_dest_addr.isNull())
        return false;

    // qDebug() << _dest_addr.toString() << _dest_port << data.size() << data.toHex().toUpper();

    return _udp.writeDatagram(data, _dest_addr, _dest_port) == data.size();
}

void UdpForwarding::readyRead()
{
    if (!active())
        return;

    if (!f_enabled->value().toBool())
        return;

    while (_udp.hasPendingDatagrams()) {
        auto datagram = _udp.receiveDatagram();
        if (!datagram.isValid())
            continue;

        if (!_pdata)
            continue;

        // qDebug() << datagram.data().size();

        _pdata->sendSerial(f_vcpid->value().toInt(), datagram.data());
    }
}
