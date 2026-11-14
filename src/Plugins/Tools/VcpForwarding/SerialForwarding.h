#pragma once

#include "PortForwarding.h"

#include <QSerialPort>

class SerialForwarding : public PortForwarding
{
    Q_OBJECT

public:
    explicit SerialForwarding(Fact *parent = nullptr);

    Fact *f_serialPort;
    Fact *f_serialPortBaudrate;

private:
    QSerialPort m_port;

    bool forwardToPHY(QByteArray data) override;
    void restorePersistent() override;

private slots:
    void readyRead();

    void updatePortsList();
    void onSerialPortChanged();
    void onSerialPortErrorOccured(QSerialPort::SerialPortError error);
};
