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
#include <serial/CobsDecoder.h>
#include <serial/CobsEncoder.h>
#include <QUdpSocket>

class DatalinkSerialRemoteUdpMcast : public DatalinkConnection
{
    Q_OBJECT

public:
    explicit DatalinkSerialRemoteUdpMcast(Fact *parent, QString mcast, int port);

    Fact *f_state = nullptr;
    Fact *f_remove = nullptr;

    QString getHost() const;
    int getPort() const;

protected:
    //DatalinkConnection overrided
    void open() override;
    void close() override;
    QByteArray read() override;
    void write(const QByteArray &packet) override;

private:
    QString m_mcast;
    QString m_host;
    int m_port;
    QUdpSocket m_readSocket;
    QUdpSocket m_writeSocket;
    QTimer m_noDataTimer;
    CobsDecoder<> m_dec;
    CobsEncoder<> m_enc;

private slots:
    void onErrorOccured(QAbstractSocket::SocketError socketError);
    void onActivatedChanged();
    void onNoDataTimerTimeout();
    void onRemoveTriggered();
};
