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
#ifndef DatalinkTcpSocket_H
#define DatalinkTcpSocket_H
#include "DatalinkConnection.h"
#include <QtCore>
#include <QtNetwork>
//=============================================================================
class DatalinkTcpSocket : public DatalinkConnection
{
    Q_OBJECT
public:
    explicit DatalinkTcpSocket(Fact *parent,
                               QTcpSocket *socket,
                               quint16 rxNetwork,
                               quint16 txNetwork);

    static bool isLocalHost(const QHostAddress address);

    QHostAddress hostAddress;
    quint16 hostPort;

private:
    QTcpSocket *socket;
    bool _serverClient;

    typedef struct
    {
        quint16 size;    //packet size
        quint16 crc16;   //packet qChecksum
        bool datalink;   //datalink stream connected
        QStringList hdr; //http header response
        QHash<QString, QString> hdr_hash;
    } SocketData;

    SocketData data;

    QString serverName;
    QString requestHdrHostName;

    bool readHeader();
    QByteArray makeTcpPacket(const QByteArray &ba) const;

    bool checkHeader();
    bool checkServerRequestHeader();
    bool checkDatalinkResponseHeader();

    void resetDataStream();

protected:
    void connectToHost(QHostAddress host, quint16 port);

    //DatalinkConnection overrided
    void close();
    QByteArray read();
    void write(const QByteArray &packet);

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
//=============================================================================
#endif
