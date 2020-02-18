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

QStringList DatalinkSerial::openPorts;

DatalinkSerial::DatalinkSerial(Fact *parent, QString devName, uint baud)
    : DatalinkConnection(parent,
                         "serial#",
                         "",
                         "",
                         Datalink::CLIENTS | Datalink::LOCAL,
                         Datalink::CLIENTS | Datalink::LOCAL)
    , m_devName(devName)
    , m_baud(baud)
    , lock(nullptr)
    , txdata(xbus::size_packet_max * 2, '\0')
    , rxdata(xbus::size_packet_max, '\0')
{
    setUrl(m_devName);

    dev = new QSerialPort(this);
    connect(dev, &QSerialPort::errorOccurred, this, &DatalinkSerial::serialPortError);
    connect(dev, &QSerialPort::readyRead, this, &DatalinkSerial::readDataAvailable);

    connect(this, &DatalinkConnection::activatedChanged, this, [this]() {
        if (activated()) {
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

void DatalinkSerial::setDevName(QString v)
{
    if (m_devName == v)
        return;
    m_devName = v;

    setUrl(v);

    if (dev->isOpen()) {
        closePort();
    }
    openNext();
}
void DatalinkSerial::setBaud(uint v)
{
    if (m_baud == v)
        return;
    m_baud = v;
}

void DatalinkSerial::openNext()
{
    if (dev->isOpen()) {
        closePort();
        return;
    }
    if (!activated())
        return;

    setStatus(tr("Searching"));

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
        if (m_devName != info.portName())
            setUrl(QString("%1:%2").arg(m_devName).arg(info.portName()));
        setStatus(tr("Connected"));
        apxMsg() << tr("Serial port connected").append(":") << info.portName();
        opened();
        return;
    }
    //continue to watch for ports to be available
    openTimer.start();
}

bool DatalinkSerial::openPort(const QSerialPortInfo &spi, uint baud)
{
    setStatus(tr("Connecting"));

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
    setStatus("");
    setUrl(m_devName);
    //if(openTimer && openTimer->isActive())openTimer->stop();
}

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

void DatalinkSerial::write(const QByteArray &packet)
{
    if (!dev->isOpen())
        return;
    if (!esc_writer.encode(packet.data(), static_cast<size_t>(packet.size()))) {
        apxConsoleW() << "esc tx overflow:" << packet.size() << esc_writer.size();
    }
    while (1) {
        size_t cnt = esc_writer.read(txdata.data(), static_cast<size_t>(txdata.size()));
        if (!cnt)
            break;
        if (dev->write(txdata.left(static_cast<int>(cnt))) <= 0) {
            serialPortError(QSerialPort::WriteError);
            esc_writer.reset();
            break;
        }
    }
}

QByteArray DatalinkSerial::read()
{
    qint64 cnt = 0;
    if (dev->isOpen())
        cnt = dev->read(reinterpret_cast<char *>(rxdata.data()), rxdata.size());
    if (cnt < 0) {
        serialPortError(QSerialPort::ReadError);
    }
    if (cnt > 0) {
        //qDebug() << cnt << rxdata.size();
        if (!esc_reader.decode(rxdata.data(), static_cast<size_t>(cnt))) {
            apxConsoleW() << "esc rx overflow:" << cnt << esc_reader.size();
        }
    }
    QByteArray packet;
    if (esc_reader.size() > 0) {
        packet.resize(static_cast<int>(esc_reader.size()));
        size_t cnt = esc_reader.read_packet(packet.data(), esc_reader.size());
        packet.resize(static_cast<int>(cnt));
    }
    if (esc_reader.size() > 0 || (dev->isOpen() && dev->bytesAvailable() > 0)) {
        QTimer::singleShot(0, this, &DatalinkSerial::readDataAvailable);
    }
    return packet;
}
