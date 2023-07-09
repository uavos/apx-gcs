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
#include <QtCore>
#include <QtNetwork>

class DatalinkUdp : public DatalinkConnection
{
    Q_OBJECT
public:
    explicit DatalinkUdp(Fact *parent,
                         QAbstractSocket *socket,
                         QHostAddress hostAddress,
                         quint16 hostPort,
                         quint16 rxNetwork,
                         quint16 txNetwork);

    static bool isLocalHost(const QHostAddress address);

private:
    QAbstractSocket *_socket;

    QHostAddress _hostAddress;
    quint16 _hostPort;

protected:
    //DatalinkConnection overrided
    virtual void close() override;
    virtual QByteArray read() override;
    virtual void write(const QByteArray &packet) override;

    // interface

private slots:
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError socketError);
    void socketStateChanged(QAbstractSocket::SocketState socketState);

signals:
    void httpRequest(QTextStream &stream, QString req, bool *ok);
    void disconnected();
    void error();
};
