#pragma once

#include "PortForwarding.h"

#include <QUdpSocket>

class UdpForwarding : public PortForwarding
{
    Q_OBJECT

public:
    explicit UdpForwarding(Fact *parent = nullptr);

    Fact *f_udp_tx;
    Fact *f_udp_rx;

private:
    QUdpSocket _udp;

    bool forwardToPHY(QByteArray data) override;

private slots:
    void readyRead();
};
