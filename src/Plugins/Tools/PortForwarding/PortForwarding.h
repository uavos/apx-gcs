#pragma once

#include <QtCore>
#include <QSerialPort>
#include "Fact/Fact.h"
#include "Vehicles/Vehicle.h"

class PortForwarding : public Fact
{
    Q_OBJECT

public:
    explicit PortForwarding(Fact *parent = nullptr);

    Fact *f_serialPort = nullptr;
    Fact *f_serialPortBaudrate = nullptr;
    Fact *f_virtualPort = nullptr;
    Fact *f_status;

private:
    QTimer m_updatePortsTimer;
    QSerialPort m_port;
    PData *m_currentPdata = nullptr;

private slots:
    void onUpdatePortsTimerTimeout();
    void onSerialPortChanged();
    void onSerialPortReadyRead();
    void onSerialPortErrorOccured(QSerialPort::SerialPortError error);
    void onCurrentVehicleChanged();
    void onPdataSerialData(quint8 portID, QByteArray data);
    void setStatus(bool ok);
};
