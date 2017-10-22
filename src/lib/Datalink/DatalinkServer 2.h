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
#ifndef DatalinkServer_H
#define DatalinkServer_H
#include <QtCore>
#include <QtNetwork>
//-----------------------------------------------------------------------------
class QTcpServer;
class QNetworkSession;
class QTcpSocket;
class QUdpSocket;
//=============================================================================
class DatalinkServer : public QObject
{
  Q_OBJECT
public:
  explicit DatalinkServer(QObject * parent = 0);

  Q_INVOKABLE const QString connectedServer(void) const;
private:
  QTcpServer *server;
  QTcpSocket *extSocket;
  typedef struct {
    quint16 size;   //packet size
    quint16 crc16;  //packet qChecksum
    bool datalink;  //datalink stream connected
    bool local;     //datalink stream connected
    QStringList hdr;//http header response
  }Client;
  QHash<QTcpSocket *,Client> connections;
  uint retryBind,retryExt;
  void httpRequest(QTcpSocket *socket);

  void sendUplinkLocal(const QByteArray &ba);
  void sendPacket(QTcpSocket *socket,const QByteArray &ba);
  void forwardPacket(QTcpSocket *src_socket,const QByteArray &ba);

  QString extServerName;
  QHostAddress lastExtServerConnected;
  QHostAddress masterHost;

  bool isLocalHost(const QHostAddress address) const;

  bool isPacketLocalForwardable(const QByteArray &ba) const;

  //udp discover
  QHash<QHostAddress,QString> servers; //with name string
  QUdpSocket *udpDiscover;
  QUdpSocket *udpDiscoverSearch;

  QByteArray makeTcpPacket(const QByteArray &ba) const;

  QTimer heartbeatTimer; //data link alive timer for udp

  QTimer connectExtTimer;

  bool bDontAutoConnectExt;
private slots:
  void tryBindServer(void);
  void newConnection();
  void socketReadyRead();
  void socketDisconnected();
  void socketError(QAbstractSocket::SocketError socketError);
  void extSocketConnected();
  void tryExtReconnect();

  //UDP discover service
  void discover(void);
  void tryBindUdpDiscover(void);
  void udpDiscoverRead(void);
  void udpDiscoverSearchRead(void);

  void heartbeatTimeout(void);

public slots:
  Q_INVOKABLE void localDataReceived(const QByteArray &ba); //connect iface/uart rx here
  Q_INVOKABLE void dataSend(const QByteArray &ba); //connect GCU tx here

  Q_INVOKABLE void connectToServer(const QHostAddress address);
  Q_INVOKABLE void connectToServer(const QString server);
  Q_INVOKABLE void connectToAny(void);

  Q_INVOKABLE void activate();

  Q_INVOKABLE void disconnect();

signals:
  void loacalDataSend(const QByteArray &ba); //connect iface/uart tx here
  void dataReceived(const QByteArray &ba); //connect GCU rx here

  void heartbeat(const QByteArray &ba); //send ping

  void httpRequest(QTextStream &stream,QString req,bool *ok);

  void serverDiscovered(const QHostAddress address,const QString name);

  //PROPERTIES
public:
  Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
  bool readOnly() const;
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
  bool active() const;
  Q_PROPERTY(bool bindEnabled READ bindEnabled WRITE setBindEnabled NOTIFY bindEnabledChanged)
  bool bindEnabled() const;
  Q_PROPERTY(bool heartbeatEnabled READ heartbeatEnabled WRITE setHeartbeatEnabled NOTIFY heartbeatEnabledChanged)
  bool heartbeatEnabled() const;
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  QString name() const;
  Q_PROPERTY(bool extctrEnabled READ extctrEnabled WRITE setExtctrEnabled NOTIFY extctrEnabledChanged)
  bool extctrEnabled() const;
  Q_PROPERTY(QStringList serverNames READ serverNames NOTIFY serverDiscovered)
  QStringList serverNames();
  Q_PROPERTY(bool extConnected READ extConnected NOTIFY extConnectedChanged)
  bool extConnected() const;
private:
  bool m_readOnly;
  bool m_active;
  bool m_bindEnabled;
  bool m_heartbeatEnabled;
  QString m_name;
  bool m_extctrEnabled;
signals:
  void readOnlyChanged(bool);
  void activeChanged(bool);
  void bindEnabledChanged(bool);
  void heartbeatEnabledChanged(bool);
  void nameChanged(QString);
  void extctrEnabledChanged(bool);
  void extConnectedChanged(bool);
public slots:
  Q_INVOKABLE void setReadOnly(bool v);
  Q_INVOKABLE void setActive(bool v);
  Q_INVOKABLE void setBindEnabled(bool v);
  Q_INVOKABLE void setHeartbeatEnabled(bool v);
  Q_INVOKABLE void setName(QString v);
  Q_INVOKABLE void setExtctrEnabled(bool v);
};
//=============================================================================
#endif
