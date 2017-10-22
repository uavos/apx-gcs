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
#ifndef Datalink_H
#define Datalink_H
#include <QtCore>
#include <QtNetwork>
#include "DatalinkFacts.h"
//-----------------------------------------------------------------------------
class QTcpServer;
class QNetworkSession;
class QTcpSocket;
class QUdpSocket;
//=============================================================================
class Datalink : public DatalinkFacts
{
  Q_OBJECT
public:
  explicit Datalink(FactSystem *parent=0);

private:
  QTcpServer *tcpServer;

  typedef struct {
    quint16 size;   //packet size
    quint16 crc16;  //packet qChecksum
    bool datalink;  //datalink stream connected
    bool local;     //datalink stream connected
    QStringList hdr;//http header response
  }Client;

  QHash<QTcpSocket *,Client> connections;

  uint retryBind;
  void httpRequest(QTcpSocket *socket);

  void sendUplinkLocal(const QByteArray &ba);
  void sendPacket(QTcpSocket *socket,const QByteArray &ba);
  void forwardPacket(QTcpSocket *src_socket,const QByteArray &ba);

  QByteArray makeTcpPacket(const QByteArray &ba) const;

  QTimer heartbeatTimer; //data link alive timer for udp

private slots:
  void tryBind(void);

  void newConnection();
  void socketReadyRead();
  void socketDisconnected();
  void socketError(QAbstractSocket::SocketError socketError);

  void heartbeatTimeout(void);

  //facts change
  void activeChanged();
  void hbeatChanged();

public slots:
  Q_INVOKABLE void localDataReceived(const QByteArray &ba); //connect iface/uart rx here
  Q_INVOKABLE void dataSend(const QByteArray &ba); //connect GCU tx here

signals:
  void loacalDataSend(const QByteArray &ba); //connect iface/uart tx here
  void dataReceived(const QByteArray &ba); //connect GCU rx here

  void heartbeat(const QByteArray &ba); //send ping

  void httpRequest(QTextStream &stream,QString req,bool *ok);

  void serverDiscovered(const QHostAddress address,const QString name);

};
//=============================================================================
#endif
