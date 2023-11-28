#include "UdpForwarding.h"

UdpForwarding::UdpForwarding(Fact *parent)
    : PortForwarding(parent,
                     "udp",
                     tr("UDP VCP forwarding"),
                     tr("Forward system UDP RX-TX ports to APX virtual port"),
                     "ip-network")
{
    f_udp_tx = new Fact(this,
                        "tx_port",
                        tr("Destination"),
                        tr("UDP to send VCP data to [IP:]<PORT>"),
                        Text | PersistentValue);
    f_udp_tx->setDefaultValue("127.0.0.1:9333");

    f_udp_rx = new Fact(this,
                        "rx_port",
                        tr("RX UDP port"),
                        tr("UDP port to receive VCP data from"),
                        Int | PersistentValue);
    f_udp_rx->setMin(0);
    f_udp_rx->setMax(65535);
    f_udp_rx->setDefaultValue(9335);

    connect(this, &Fact::activeChanged, this, [this]() {
        if (!active()) {
            _udp.close();
            apxMsg() << "UDP" << f_udp_rx->value().toUInt() << tr("closed");
        }
    });

    connect(f_enabled, &Fact::valueChanged, this, [this]() {
        if (f_enabled->value().toBool()) {
            if (!_udp.bind(f_udp_rx->value().toUInt())) {
                apxMsgW() << tr("Failed to bind UDP port") << f_udp_rx->value().toUInt();
                QTimer::singleShot(0, this, [this]() { setActive(false); });
            } else {
                setActive(true);
                apxMsg() << "UDP" << f_udp_rx->value().toUInt() << tr("bound");
            }
        } else {
            qDebug() << "UDP" << f_udp_rx->value().toUInt() << tr("shutdown");
            setActive(false);
        }
    });

    connect(&_udp, &QUdpSocket::readyRead, this, &UdpForwarding::readyRead);
}

bool UdpForwarding::forwardToPHY(QByteArray data)
{
    QHostAddress dest = QHostAddress::LocalHost;
    quint16 port = 0;

    auto s = f_udp_tx->value().toString();
    if (s.contains(':')) {
        auto p = s.split(':');
        if (p.size() != 2)
            return false;
        dest = QHostAddress(p.at(0));
        port = p.at(1).toUInt();
    }

    if (!port)
        return false;

    if (dest.isNull())
        return false;

    return _udp.writeDatagram(data, dest, port) == data.size();
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

        auto data = datagram.data();
        if (_pdata) {
            _pdata->sendSerial(f_vcpid->value().toInt(), data);
        }
    }
}
