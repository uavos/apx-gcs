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

#include "QueueItem.h"

#include <QSerialPort>
#include <QtCore>

class QueueItem;

class LoaderStm : public QueueItem
{
    Q_OBJECT

public: //186
    explicit LoaderStm(
        Fact *parent, QString name, QString hw, QString type, QString portName, bool continuous);
    ~LoaderStm() override;

private:
    QString portName;
    bool continuous;

    QTimer timer;
    int stage;
    bool success;

    QSerialPort *dev{nullptr};
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

protected:
    void upload() override;

private slots:
    void next();
    void readData();

public slots:
    void stop();
};
