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
#pragma once

#include "DatalinkConnection.h"

#include <Xbus/XbusPacket.h>

#include <Xbus/uart/SerialCodec.h>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCore>

class DatalinkSerial : public DatalinkConnection
{
    Q_OBJECT
    Q_ENUMS(CodecType)

public:
    explicit DatalinkSerial(Fact *parent, QString devName, uint baud);

    enum CodecType { COBS, ESC, RAW };
    Q_ENUM(CodecType)

    void setDevName(QString v);
    void setBaud(uint v);
    void setCodec(CodecType v);

private:
    QString m_devName;
    uint m_baud;

    QTimer rxTimer;

    QSerialPort *dev;
    QSerialPortInfo info;

    QLockFile *lock;
    static QStringList openPorts;
    bool isAvailable(const QSerialPortInfo &spi);
    bool openPort(const QSerialPortInfo &spi, uint baud);
    void closePort();

    QTimer openTimer;
    int scanIdx;

    SerialEncoder *encoder;
    SerialDecoder *decoder;

    QByteArray txdata;
    QByteArray rxdata;

protected:
    //DatalinkConnection overrided
    void open() override;
    void close() override;
    QByteArray read() override;
    void write(const QByteArray &packet) override;

private slots:
    void openNext();
    void serialPortError(QSerialPort::SerialPortError error);
};
