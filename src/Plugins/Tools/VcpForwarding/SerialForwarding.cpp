#include "SerialForwarding.h"

#include <QSerialPort>
#include <QSerialPortInfo>

SerialForwarding::SerialForwarding(Fact *parent)
    : PortForwarding(parent,
                     "serial",
                     tr("Serial VCP forwarding"),
                     tr("Forward system serial port to APX virtual port"),
                     "serial-port")
{
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
    connect(this, &Fact::triggered, this, &SerialForwarding::updatePortsList);

    connect(f_serialPort, &Fact::valueChanged, this, &SerialForwarding::onSerialPortChanged);
    connect(f_serialPortBaudrate, &Fact::valueChanged, this, &SerialForwarding::onSerialPortChanged);

    connect(&m_port, &QSerialPort::readyRead, this, &SerialForwarding::readyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, &SerialForwarding::onSerialPortErrorOccured);

    connect(f_enabled, &Fact::valueChanged, this, &SerialForwarding::onSerialPortChanged);

    connect(this, &Fact::activeChanged, this, [this]() {
        if (!active()) {
            m_port.close();
        }
    });

    QTimer::singleShot(1234, this, &SerialForwarding::updatePortsList);
}

void SerialForwarding::updatePortsList()
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

void SerialForwarding::onSerialPortChanged()
{
    if (m_port.isOpen()) {
        m_port.close();
    }

    if (!f_enabled->value().toBool()) {
        setActive(false);
        return;
    }

    m_port.setPortName(f_serialPort->text());
    m_port.setBaudRate(f_serialPortBaudrate->value().toInt());
    if (m_port.open(QIODevice::ReadWrite)) {
        setActive(true);
    } else {
        setActive(false);
    }
}

void SerialForwarding::readyRead()
{
    if (!active())
        return;

    if (!f_enabled->value().toBool())
        return;

    auto data = m_port.readAll();
    if (_pdata) {
        _pdata->sendSerial(f_vcpid->value().toInt(), data);
    }
}

void SerialForwarding::onSerialPortErrorOccured(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        setActive(false);
        apxMsgW() << "SerialForwarding: serial port error " << error;
    }
}

bool SerialForwarding::forwardToPHY(QByteArray data)
{
    while (!data.isEmpty()) {
        auto result = m_port.write(data);
        if (result <= 0)
            return false;

        data.remove(0, result);
    }
    return true;
}
