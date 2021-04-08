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
#include "LoaderStm.h"

#include <App/App.h>
#include <App/AppGcs.h>
#include <App/AppLog.h>
#include <QSerialPortInfo>

#define SLDR_ACK 0x79
#define SLDR_NACK 0x1F

LoaderStm::LoaderStm(
    Fact *parent, QString name, QString hw, QString type, QString portName, bool continuous)
    : QueueItem(parent, "", name, hw, type)
    , portName(portName)
    , continuous(continuous)
{
    setTitle(name);

    timer.setSingleShot(true);
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, this, &LoaderStm::next);
}
LoaderStm::~LoaderStm()
{
    if (dev) {
        dev->close();
        AppGcs::instance()->f_datalink->f_ports->unblockSerialPorts();
    }
}

void LoaderStm::upload()
{
    if (!loadFirmware(_hw)) {
        finish(false);
        return;
    }

    AppGcs::instance()->f_datalink->f_ports->blockSerialPorts();

    dev = new QSerialPort(QSerialPortInfo(portName), this);
    connect(dev, &QSerialPort::readyRead, this, &LoaderStm::readData);

    stage = 0;
    success = false;
    retry = 0;
    timer.start();
}

void LoaderStm::stop()
{
    timer.stop();
}

void LoaderStm::next()
{
    switch (stage) {
    default:
        break;

    case 0: //open port
        dev->setBaudRate(115200);
        dev->setParity(QSerialPort::EvenParity);
        retry = 0;
        stage++;
        //fallthru
        [[clang::fallthrough]];
    case 1:
        if (dev->open(QIODevice::ReadWrite)) {
            apxMsg() << tr("Using port").append(':') << dev->portName();
            if (continuous)
                apxMsg() << tr("Continuous mode");
            stage = 10;
            timer.start(200);
            return;
        }
        //repeat
        retry++;
        if (retry > 10) {
            apxMsgW() << tr("Can't open port") << dev->portName() << dev->error();
            break;
        }
        timer.start(500);
        return;

    // port opened
    case 10: //init loader
        apxMsg() << tr("Initializing").append("...");
        setProgress(0);
        write(QByteArray(1, 0x7F), rx_loader, 100);
        retry = 0;
        stage++;
        return;
    case 11:
        if (rx_stage == rx_ok) {
            qDebug() << "Loader OK";
            stage = 20;
            timer.start(100);
            return;
        }
        //repeat
        if (!continuous)
            retry++;
        if (retry > 150) {
            apxMsgW() << tr("Can't initialize stm loader");
            break;
        }
        write(QByteArray(1, 0x7F), rx_loader, 100);
        return;

    // in loader
    case 20:
        //read chip info
        write(0x00, rx_info);
        stage++;
        return;
    case 21:
        if (rx_stage == rx_ok) {
            stage = 30;
            timer.start(0);
            return;
        }
        //repeat
        retry++;
        if (retry > 5) {
            apxMsgW() << tr("No response from chip");
            break;
        }
        write(0x00, rx_info);
        return;

    // erase chip
    case 30:
        if (_offset != 0x08000000) {
            /*apxMsg() << tr("Chip erase skipped");
            stage = 50;
            timer.start(0);
            return;*/
            apxMsg() << tr("Offset").append(':')
                     << QString("0x%1").arg(static_cast<qulonglong>(_offset), 8, 16, QChar('0'));
        }
        apxMsg() << tr("Erasing chip").append("...");
        write(cmd_erase, rx_ack);
        stage++;
        return;
    case 31:
        if (rx_stage != rx_ok) {
            apxMsgW() << tr("Command error");
            break;
        }
        if (cmd_erase == 0x43) { //standard
            write(0xFF, rx_ack, 60000);
        } else { //extended
            write(crc(QByteArray(2, 0xFF)), rx_ack, 120000);
        }
        stage++;
        return;
    case 32:
        if (rx_stage != rx_ok) {
            apxMsgW() << tr("Chip erase error");
            break;
        }
        qDebug() << "Chip erased";
        stage = 50;
        timer.start(0);
        return;

    //write data
    case 50:
        apxMsg() << tr("Uploading data").append("...");
        writeCnt = 0;
        stage++;
        //fallthru
        [[clang::fallthrough]];
    case 51:
        write(0x31, rx_ack);
        stage++;
        return;
    case 52:
        if (rx_stage == rx_ok) {
            QByteArray ba;
            quint32 writeAddr = _offset + writeCnt;
            ba.append(static_cast<char>(writeAddr >> 24));
            ba.append(static_cast<char>(writeAddr >> 16));
            ba.append(static_cast<char>(writeAddr >> 8));
            ba.append(static_cast<char>(writeAddr));
            write(crc(ba), rx_ack);
            stage++;
            return;
        }
        apxMsgW() << tr("Write command error");
        break;
    case 53:
        if (rx_stage == rx_ok) {
            QByteArray ba = _data.mid(writeCnt, 256);
            writeCnt += ba.size();
            while ((ba.size() & 3) != 0)
                ba.append(static_cast<char>(0xFF));
            ba.prepend(ba.size() - 1);
            write(crc(ba), rx_ack);
            setProgress(100 * writeCnt / _data.size());
            stage++;
            return;
        }
        apxMsgW() << tr("Write address error");
        break;
    case 54:
        if (rx_stage == rx_ok) {
            //qDebug() << "written" << writeCnt;
            if (writeCnt >= _data.size()) {
                stage = 100;
                timer.start(0);
                return;
            }
            stage = 51;
            timer.start(0);
            return;
        }
        apxMsgW() << tr("Write data error");
        break;

    // start program
    case 100:
        //apxMsg() << tr("Starting program");
        write(0x21, rx_ack);
        stage++;
        return;
    case 101:
        if (rx_stage == rx_ok) {
            QByteArray ba;
            ba.append(static_cast<char>(_offset >> 24));
            ba.append(static_cast<char>(_offset >> 16));
            ba.append(static_cast<char>(_offset >> 8));
            ba.append(static_cast<char>(_offset));
            write(crc(ba), rx_ack);
            stage++;
            return;
        }
        apxMsgW() << tr("Write program address error");
        break;
    case 102:
        if (rx_stage == rx_ok) {
            success = true;
        } else {
            apxMsgW() << tr("Start program error");
        }
        break;
    }

    // retry forever
    if (stage >= 20 && continuous) {
        if (!success)
            qWarning() << "retry" << stage << rx_stage << time.elapsed();
        else {
            apxMsg() << tr("Success");
            apxMsg() << tr("Next chip").append("...");
        }
        stage = 10;
        timer.start(200);
        return;
    }

    //exit
    qDebug() << stage << rx_stage << time.elapsed();
    deleteLater();
}

void LoaderStm::readData()
{
    rxData.append(dev->readAll());
    if (rxData.isEmpty())
        return;
    switch (rx_stage) {
    default:
        break;
    case rx_ack:
        if (rxData == QByteArray(rxData.size(), rxData.at(0))) {
            quint8 v = rxData.at(0);
            if (v == SLDR_ACK) {
                rx_stage = rx_ok;
                break;
            }
            apxMsgW() << tr("Wrong response from chip");
            qDebug() << "wrong response" << rxData.toHex().toUpper();
            timer.start(0);
        }
        break;
    case rx_loader:
        if (rxData == QByteArray(rxData.size(), rxData.at(0))) {
            quint8 v = rxData.at(0);
            if (v == SLDR_ACK || v == SLDR_NACK) {
                rx_stage = rx_ok;
                break;
            }
        }
        break;
    case rx_info: {
        quint8 v = rxData.at(0);
        if (v != SLDR_ACK) {
            timer.start(0);
            break;
        }
        rxData.remove(0, 1);
        rx_stage = rx_info_data;
    } //fallthru
        [[clang::fallthrough]];
    case rx_info_data: {
        if (rxData.size() < 2)
            return;
        int sz = static_cast<quint8>(rxData.at(0)) + 3;
        if (rxData.size() < sz)
            return;
        quint8 vAck = rxData.at(rxData.size() - 1);
        if (rxData.size() != sz || vAck != SLDR_ACK) {
            qDebug() << "wrong response" << rxData.toHex().toUpper();
            timer.start(0);
            break;
        }
        rxData.remove(0, 1);
        rx_stage = rx_ok;
        quint8 ver = rxData.at(0);
        apxMsg() << QString("STM Bootloader v%1.%2").arg(ver >> 4).arg(ver & 0x0F);
        cmd_erase = rxData.at(7);
    } break;
    }

    if (rx_stage == rx_ok) {
        retry = 0;
        timer.start(0);
    }
    rxData.clear();
}

void LoaderStm::write(const QByteArray &data, RxStage rx_stage, int timeout)
{
    time.start();
    this->rx_stage = rx_stage;
    rxData.clear();
    dev->write(data);
    if (rx_stage != rx_ok)
        timer.start(timeout);
}
void LoaderStm::write(quint8 cmd, RxStage rx_stage, int timeout)
{
    QByteArray ba;
    ba.append(static_cast<char>(cmd));
    ba.append(static_cast<char>(~cmd));
    write(ba, rx_stage, timeout);
}

QByteArray LoaderStm::crc(QByteArray data) const
{
    quint8 crc = 0;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<quint8>(data.at(i));
    }
    data.append(static_cast<char>(crc));
    return data;
}
