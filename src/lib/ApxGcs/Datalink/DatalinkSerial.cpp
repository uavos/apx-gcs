/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "DatalinkSerial.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>
//-----------------------------------------------------------------------------
QStringList DatalinkSerial::openPorts;
//=============================================================================
DatalinkSerial::DatalinkSerial(Fact *parent, QString devName, uint baud)
    : DatalinkConnection(parent,
                         "serial#",
                         "",
                         "",
                         Datalink::CLIENTS | Datalink::LOCAL,
                         Datalink::CLIENTS | Datalink::LOCAL)
    , Escaped()
    , m_devName(devName)
    , m_baud(baud)
    , lock(nullptr)
{
    dev = new QSerialPort(this);
    connect(dev, &QSerialPort::errorOccurred, this, &DatalinkSerial::serialPortError);
    connect(dev, &QSerialPort::readyRead, this, &DatalinkSerial::readDataAvailable);

    connect(this, &Fact::valueChanged, this, [this]() {
        if (value().toBool()) {
            open();
        } else {
            close();
        }
    });

    scanIdx = 0;
    openTimer.setSingleShot(true);
    openTimer.setInterval(800);
    connect(&openTimer, &QTimer::timeout, this, &DatalinkSerial::openNext);
}
//=============================================================================
void DatalinkSerial::setDevName(QString v)
{
    if (m_devName == v)
        return;
    m_devName = v;
}
void DatalinkSerial::setBaud(uint v)
{
    if (m_baud == v)
        return;
    m_baud = v;
}
//=============================================================================
void DatalinkSerial::openNext()
{
    if (dev->isOpen()) {
        closePort();
        return;
    }
    if (!value().toBool())
        return;

    while (1) {
        QSerialPortInfo spi;
        if ((!m_devName.isEmpty()) && m_devName != "auto") {
            //the port name is m_devName
            spi = QSerialPortInfo(m_devName);
        } else {
            //[auto] - scan for any next available port
            QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
            if (list.isEmpty())
                break;
            if (scanIdx >= list.size())
                scanIdx = 0;
            for (int i = 0; i < list.size(); i++) {
                const QSerialPortInfo &lspi = list.at(scanIdx);
                if (lspi.portName().contains("usb", Qt::CaseInsensitive)
                    && lspi.portName().contains("cu.", Qt::CaseInsensitive) && isAvailable(lspi)) {
                    spi = lspi;
                    break;
                }
                scanIdx++;
                if (scanIdx >= list.size())
                    scanIdx = 0;
            }
        }
        if (!isAvailable(spi))
            break;
        //try to open valid spi port
        //qDebug("Trying to open %s (%s)",spi.portName().toUtf8().data(),spi.systemLocation().toUtf8().data());
        if (!openPort(spi, m_baud)) {
            apxMsgW() << tr("Serial port open failed") << QString("(%1)").arg(spi.portName());
            break;
        }
        info = spi;
        openPorts.append(info.portName());
        setTitle(info.portName());
        apxMsg() << tr("Serial port connected").append(":") << info.portName();
        opened();
        return;
    }
    //continue to watch for ports to be available
    openTimer.start();
}
//=============================================================================
bool DatalinkSerial::openPort(const QSerialPortInfo &spi, uint baud)
{
    dev->setPort(spi);
    dev->setBaudRate(9600);
    if (!dev->open(QIODevice::ReadWrite))
        return false;
    dev->setBaudRate(static_cast<int>(baud));
    //lock
    if (lock) {
        lock->unlock();
        delete lock;
    }
    lock = new QLockFile(
        QDir::tempPath() + "/"
        + QString(QCryptographicHash::hash(QByteArray(spi.systemLocation().toUtf8()),
                                           QCryptographicHash::Sha1)
                      .toHex())
        + ".lock");
    if (!lock->tryLock()) {
        closePort();
        apxMsgW() << "Unable to lock" << spi.systemLocation();
        return false;
    }
    return true;
}
void DatalinkSerial::closePort()
{
    if (lock) {
        lock->unlock();
        delete lock;
        lock = nullptr;
    }
    if (dev->isOpen())
        dev->close();
    openPorts.removeAll(info.portName());
    closed();
    //if(openTimer && openTimer->isActive())openTimer->stop();
}
//=============================================================================
bool DatalinkSerial::isAvailable(const QSerialPortInfo &spi)
{
    if (spi.isNull())
        return false;
    if (openPorts.contains(spi.portName()))
        return false;

    QLockFile tlock(QDir::tempPath() + "/"
                    + QString(QCryptographicHash::hash(QByteArray(spi.systemLocation().toUtf8()),
                                                       QCryptographicHash::Sha1)
                                  .toHex())
                    + ".lock");
    if (!tlock.tryLock(10))
        return false;
    tlock.unlock();
    return true;
}
//=============================================================================
void DatalinkSerial::serialPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    apxConsoleW() << "Serial error:" << error;
    dev->clearError();
    if (dev->isOpen()) {
        closePort();
        apxMsgW() << tr("Serial port disconnected").append(":") << info.portName();
    }
    openTimer.start();
}
//=============================================================================
void DatalinkSerial::open()
{
    //qDebug()<<active();
    openNext();
}
void DatalinkSerial::close()
{
    //qDebug()<<active();
    if (dev->isOpen()) {
        apxMsg() << tr("Serial port closed").append(":") << info.portName();
    }
    closePort();
}
//=============================================================================
QByteArray DatalinkSerial::read()
{
    QByteArray packet;
    if (!dev->isOpen())
        return packet;
    uint cnt = readEscaped();
    if (cnt > 0) {
        packet = QByteArray(reinterpret_cast<const char *>(esc_rx), static_cast<int>(cnt));
    }
    if (dev->isOpen() && dev->bytesAvailable() > 0) {
        QTimer::singleShot(0, this, &DatalinkSerial::readDataAvailable);
    }
    return packet;
}
//=============================================================================
void DatalinkSerial::write(const QByteArray &packet)
{
    if (!dev->isOpen())
        return;
    writeEscaped(reinterpret_cast<const uint8_t *>(packet.data()), static_cast<uint>(packet.size()));
}
//=============================================================================
//=============================================================================
uint DatalinkSerial::esc_read(uint8_t *buf, uint sz)
{
    if (!dev->isOpen())
        return 0;
    qint64 cnt = dev->read(reinterpret_cast<char *>(buf), sz);
    //int cnt=::read(fd,buf,sz);
    if (cnt >= 0)
        return static_cast<uint>(cnt);
    //read error
    serialPortError(QSerialPort::ReadError);
    return 0;
}
bool DatalinkSerial::esc_write_byte(const uint8_t v)
{
    txdata.append(static_cast<char>(v));
    return true;
}
void DatalinkSerial::escWriteDone(void)
{
    if (txdata.isEmpty())
        return;
    if (dev->isOpen()) {
        if (dev->write(txdata) <= 0)
            serialPortError(QSerialPort::WriteError);
    }
    txdata.clear();
}
//=============================================================================
