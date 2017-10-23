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
#ifndef DatalinkSocket_H
#define DatalinkSocket_H
#include <QtCore>
#include <QtNetwork>
#include "FactSystem.h"
//=============================================================================
class DatalinkSocket: public Fact
{
  Q_OBJECT
public:
  explicit DatalinkSocket(Fact *parent, QString title, QTcpSocket *socket, bool bServer, QString serverName);

protected:

  QByteArray makeTcpPacket(const QByteArray &ba) const;

  QTcpSocket *socket;

  bool bServer;

  bool readHeader();
  bool checkDatalinkRequestHeader();
  bool checkDatalinkResponseHeader();

  static bool isLocalHost(const QHostAddress address);

private:
  typedef struct {
    quint16 size;   //packet size
    quint16 crc16;  //packet qChecksum
    bool datalink;  //datalink stream connected
    QStringList hdr;//http header response
    QHash<QString,QString> hdr_hash;
  }SocketData;

  SocketData data;
  QString serverName;

private slots:
  void socketDisconnected();
  void socketError(QAbstractSocket::SocketError socketError);
  void socketReadyRead();
  void socketClientConnected();

signals:
  void packetReceived(const QByteArray &ba);
  void datalinkConnected();
  void disconnected();
  void httpRequest(QTextStream &stream,QString req,bool *ok);

public slots:
  void sendPacket(const QByteArray &ba);
  void disconnectSocket();
  void readDatalinkData();
};
//=============================================================================
#endif
