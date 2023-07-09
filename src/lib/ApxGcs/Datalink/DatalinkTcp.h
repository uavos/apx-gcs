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

#include <uart/CobsDecoder.h>
#include <uart/CobsEncoder.h>

class DatalinkTcp : public DatalinkConnection
{
    Q_OBJECT
public:
    explicit DatalinkTcp(Fact *parent, QTcpSocket *socket, quint16 rxNetwork, quint16 txNetwork);

    static bool isLocalHost(const QHostAddress address);

    QHostAddress hostAddress;
    quint16 hostPort;

private:
    QTcpSocket *socket;
    bool _serverClient;

    typedef struct
    {
        bool datalink;   //datalink stream connected
        QStringList hdr; //http header response
        QHash<QString, QString> hdr_hash;
    } SocketData;

    SocketData data;

    QString serverName;
    QString requestHdrHostName;

    CobsDecoder<> _decoder;
    CobsEncoder<> _encoder;

    bool readHeader();

    bool checkHeader();
    bool checkServerRequestHeader();
    bool checkDatalinkResponseHeader();

protected:
    void connectToHost(QHostAddress host, quint16 port);

    //DatalinkConnection overrided
    void resetDataStream() override;
    void close() override;
    QByteArray read() override;
    void write(const QByteArray &packet) override;

private slots:
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError socketError);

    void readyReadHeader();
    void requestDatalinkHeader();
    void socketStateChanged(QAbstractSocket::SocketState socketState);

signals:
    void httpRequest(QTextStream &stream, QString req, bool *ok);
    void disconnected();
    void error();
};
