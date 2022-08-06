/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "DatalinkSerial.h"
#include "Datalink.h"

#include <App/App.h>
#include <App/AppLog.h>

#include <uart/CobsDecoder.h>
#include <uart/CobsEncoder.h>

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdelete-abstract-non-virtual-dtor"
#endif

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

void DatalinkSerial::setCodec(CodecType v)
{
    if (encoder) {
        delete encoder;
        encoder = nullptr;
    }
    if (decoder) {
        delete decoder;
        decoder = nullptr;
    }
    switch (v) {
    default:
        break;
    case COBS:
        encoder = new CobsEncoder();
        decoder = new CobsDecoder();
        break;
    }
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

    if (!encoder) {
        qWarning() << "not supported";
        return;
    }

    auto cnt = encoder->encode(packet.data(), static_cast<size_t>(packet.size()));
    if (cnt <= 0) {
        apxConsoleW() << "tx encode:" << packet.size() << cnt;
        return;
    }

    auto wcnt = dev->write((const char *) encoder->data(), cnt);

    if (wcnt <= 0) {
        qWarning() << "tx" << wcnt;
        serialPortError(QSerialPort::WriteError);
    }

    // qDebug() << wcnt;
}

QByteArray DatalinkSerial::read()
{
    if (!dev->isOpen()) {
        if (decoder)
            decoder->reset();
        _rx_fifo.reset();
        return {};
    }

    // first return already decoded packets
    if (!_rx_fifo.empty()) {
        auto cnt = _rx_fifo.read_packet(_rx_pkt.data(), _rx_pkt.size());
        if (cnt > 0) {
            QTimer::singleShot(0, this, &DatalinkSerial::readDataAvailable);
            return _rx_pkt.left(cnt);
        }
    }

    if (dev->bytesAvailable() <= 0)
        return {};

    // qDebug() << "rx" << dev->bytesAvailable();

    // read PHY and continue decoding
    auto rcnt = dev->read(reinterpret_cast<char *>(_rxbuf_raw), sizeof(_rxbuf_raw));
    if (rcnt < 0) {
        serialPortError(QSerialPort::ReadError);
    }
    if (!decoder) {
        qWarning() << "data stream not supported";
        return {};
    }
    if (rcnt <= 0) {
        qWarning() << "rx empty" << rcnt;
        return {};
    }

    // schedule reads for next packet
    QTimer::singleShot(0, this, &DatalinkSerial::readDataAvailable);

    // read bytes count (rcnt) might be more than one packet
    // decode all packets from raw data into fifo
    auto src = _rxbuf_raw;
    size_t cnt = rcnt;
    bool pkt = false;
    while (cnt > 0) {
        // feed the decoder
        auto decoded_cnt = decoder->decode(src, cnt);
        if (decoded_cnt <= 0)
            qWarning() << "rx decode:" << cnt << decoded_cnt;

        src += decoded_cnt;
        cnt -= decoded_cnt;

        // check if some pkt is available
        switch (decoder->status()) {
        case SerialDecoder::PacketAvailable:
            pkt = true;
            if (!_rx_fifo.write_packet(decoder->data(), decoder->size())) {
                qWarning() << "rx fifo full";
            }
            break;
        case SerialDecoder::DataAccepted:
        case SerialDecoder::DataDropped:
            break;
        default:
            apxConsoleW() << "SerialDecoder rx err:" << decoder->status() << rcnt << decoded_cnt;
            break;
        }
    }

    if (!pkt)
        return {};

    return _rx_pkt.left(_rx_fifo.read_packet(_rx_pkt.data(), _rx_pkt.size()));
}
