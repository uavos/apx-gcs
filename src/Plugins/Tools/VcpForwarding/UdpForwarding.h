#pragma once

#include "PortForwarding.h"

#include <QUdpSocket>

class UdpForwarding : public PortForwarding
{
    Q_OBJECT

public:
    explicit UdpForwarding(Fact *parent = nullptr);

    Fact *f_udp_rx;
    Fact *f_udp_tx;

private:
    QUdpSocket _udp;

    void updateDest();
    QHostAddress _dest_addr;
    quint16 _dest_port{};

    bool forwardToPHY(QByteArray data) override;

private slots:
    void readyRead();
};
