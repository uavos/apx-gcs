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
#ifndef DatalinkClient_H
#define DatalinkClient_H
#include <QtCore>
#include <QtNetwork>
#include "DatalinkFacts.h"
//=============================================================================
class DatalinkClient : public Fact
{
  Q_OBJECT
public:
  explicit DatalinkClient(DatalinkFacts *datalink,QTcpSocket *socket);

  static QByteArray makeTcpPacket(const QByteArray &ba);

private:
  DatalinkFacts *datalink;
  QTcpSocket *socket;


  typedef struct {
    quint16 size;   //packet size
    quint16 crc16;  //packet qChecksum
    bool datalink;  //datalink stream connected
    QStringList hdr;//http header response
  }ClientData;

  ClientData clientData;
  QString serverName;

private slots:
  void socketConnected();
  void socketReadyRead();
  void socketDisconnected();
  void socketError(QAbstractSocket::SocketError socketError);
  void httpRequest();

signals:
  void dataReceived(const QByteArray &ba);
  void datalinkConnected();
  void disconnected();
public slots:
  void sendPacket(const QByteArray &ba);
};
//=============================================================================
#endif
