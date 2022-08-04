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
#pragma once

#include "DatalinkConnection.h"

#include <XbusPacket.h>

#include <uart/SerialCodec.h>

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCore>

class DatalinkSerial : public DatalinkConnection
{
    Q_OBJECT
    Q_ENUMS(CodecType)

public:
    explicit DatalinkSerial(Fact *parent, QString devName, uint baud);

    enum CodecType { COBS, RAW };
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
