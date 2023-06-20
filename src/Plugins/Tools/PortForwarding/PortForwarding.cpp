#include "PortForwarding.h"

#include <QSerialPort>
#include <QSerialPortInfo>

#include "App/AppGcs.h"
#include "Vehicles/Vehicles.h"

using namespace std::chrono_literals;

PortForwarding::PortForwarding(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Port forwarding"),
           tr("Forward system serial port to APX virtual port"),
           Group,
           "swap-vertical")
{
    f_enabled
        = new Fact(this, "enable", tr("Enabled"), tr("Enable port forwarding"), Fact::Bool, "link");

    f_serialPort = new Fact(this,
                            "serial_port",
                            tr("Serial port"),
                            tr("Serial port path"),
                            Fact::Enum | Fact::PersistentValue,
                            "serial-port");
    f_serialPortBaudrate = new Fact(this,
                                    "serial_port_baudrate",
                                    tr("Baudrate"),
                                    tr("Serial port baudrate"),
                                    Fact::Int | Fact::PersistentValue,
                                    "speedometer");
    f_serialPortBaudrate->setDefaultValue(115200);
    f_virtualPort = new Fact(this,
                             "virtual_port",
                             tr("Virtual port"),
                             tr("Virtual port ID"),
                             Fact::Int | Fact::PersistentValue,
                             "numeric");
    f_virtualPort->setMin(0);
    f_virtualPort->setMax(255);

    m_updatePortsTimer.setSingleShot(false);
    m_updatePortsTimer.setInterval(3s);
    m_updatePortsTimer.start();
    connect(&m_updatePortsTimer, &QTimer::timeout, this, &PortForwarding::onUpdatePortsTimerTimeout);
    connect(f_serialPort, &Fact::valueChanged, this, &PortForwarding::onSerialPortChanged);
    connect(f_serialPortBaudrate, &Fact::valueChanged, this, &PortForwarding::onSerialPortChanged);

    connect(&m_port, &QSerialPort::readyRead, this, &PortForwarding::onSerialPortReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, &PortForwarding::onSerialPortErrorOccured);

    connect(f_enabled, &Fact::valueChanged, this, &PortForwarding::onEnabledChanged);

    onUpdatePortsTimerTimeout();
    onSerialPortChanged();
    onCurrentVehicleChanged();
}

void PortForwarding::onUpdatePortsTimerTimeout()
{
    auto ports = QSerialPortInfo::availablePorts();
    QStringList portNames;
    for (const auto &p : ports) {
        portNames.append(p.portName());
    }
    if (portNames != f_serialPort->enumStrings()) {
        f_serialPort->setEnumStrings(portNames);
    }
}

void PortForwarding::onSerialPortChanged()
{
    if (m_port.isOpen()) {
        m_port.close();
    }

    if (!f_enabled->value().toBool()) {
        setStatus(false);
        return;
    }

    m_port.setPortName(f_serialPort->text());
    m_port.setBaudRate(f_serialPortBaudrate->value().toInt());
    if (m_port.open(QIODevice::ReadWrite)) {
        setStatus(true);
    } else {
        setStatus(false);
    }
}

void PortForwarding::onSerialPortReadyRead()
{
    auto data = m_port.readAll();
    if (m_currentPdata) {
        m_currentPdata->sendSerial(f_virtualPort->value().toInt(), data);
    }
}

void PortForwarding::onSerialPortErrorOccured(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        setStatus(false);
        apxMsgW() << "PortForwarding: serial port error " << error;
    }
}

void PortForwarding::onCurrentVehicleChanged()
{
    if (m_currentPdata) {
        disconnect(m_currentPdata, &PData::serialData, this, &PortForwarding::onPdataSerialData);
    }
    auto protocol = Vehicles::instance()->current()->protocol();
    if (protocol)
        m_currentPdata = protocol->data();
    if (m_currentPdata) {
        connect(m_currentPdata, &PData::serialData, this, &PortForwarding::onPdataSerialData);
    }
}

void PortForwarding::onPdataSerialData(quint8 portID, QByteArray data)
{
    if (!f_enabled->value().toBool())
        return;

    if (portID == f_virtualPort->value().toInt()) {
        while (!data.isEmpty()) {
            auto result = m_port.write(data);
            if (result >= 0) {
                data.remove(0, result);
            } else {
                apxMsgW() << "PortForwarding: can't write data to serial port";
                setStatus(false);
                break;
            }
        }
    }
}

void PortForwarding::setStatus(bool ok)
{
    if (ok) {
        setText(QString("VCP#%1").arg(f_virtualPort->value().toInt()));
    } else {
        if (!f_enabled->value().toBool()) {
            setText("");
        } else {
            setText("Error");
            QTimer::singleShot(3s, this, &PortForwarding::onSerialPortChanged);
        }
    }
}

void PortForwarding::onEnabledChanged()
{
    onSerialPortChanged();

    f_virtualPort->setEnabled(!f_enabled->value().toBool());
}
