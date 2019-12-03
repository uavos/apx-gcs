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
#ifndef DatalinkSerial_H
#define DatalinkSerial_H
#include "DatalinkConnection.h"
#include <Xbus/uart/Escaped.h>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCore>
//=============================================================================
class DatalinkSerial : public DatalinkConnection, public Escaped
{
    Q_OBJECT
public:
    explicit DatalinkSerial(Fact *parent, QString devName, uint baud);

    void setDevName(QString v);
    void setBaud(uint v);

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

    QByteArray txdata;

protected:
    //DatalinkConnection overrided
    void open();
    void close();
    QByteArray read();
    void write(const QByteArray &packet);

    //esc
    uint esc_read(uint8_t *buf, uint sz);
    bool esc_write_byte(const uint8_t v);
    void escWriteDone(void);

private slots:
    void openNext();
    void serialPortError(QSerialPort::SerialPortError error);
};
//=============================================================================
#endif
