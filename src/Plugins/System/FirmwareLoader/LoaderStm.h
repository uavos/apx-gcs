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
#ifndef LoaderStm_H
#define LoaderStm_H
//=============================================================================
#include <QSerialPort>
#include <QtCore>
class Loader;
//=============================================================================
class LoaderStm : public QObject
{
    Q_OBJECT

public:
    explicit LoaderStm(Loader *loader,
                       const QByteArray &fileData,
                       quint32 startAddr,
                       const QString &portName,
                       bool continuous);
    ~LoaderStm();

private:
    Loader *loader;
    QByteArray fileData;
    quint32 startAddr;
    QString portName;
    bool continuous;

    QTimer timer;
    int stage;
    bool success;

    QSerialPort *dev;
    int retry;
    QByteArray rxData;
    enum RxStage {
        rx_ok,
        rx_ack,
        rx_loader,
        rx_info,
        rx_info_data,
    };
    RxStage rx_stage;
    QElapsedTimer time;

    quint8 cmd_erase;
    int writeCnt;

    void write(const QByteArray &data, RxStage rx_stage = rx_ok, int timeout = 200);
    void write(quint8 cmd, RxStage rx_stage = rx_ok, int timeout = 200);

    QByteArray crc(QByteArray data) const;

private slots:
    void next();
    void readData();

public slots:
    void stop();
};
//=============================================================================
#endif
