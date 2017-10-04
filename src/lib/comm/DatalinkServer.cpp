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
#include "DatalinkServer.h"
#include "tcp_ports.h"
#include <QtNetwork>
#include "MandalaCore.h"
#include "node.h"
#include "crc.h"
//-----------------------------------------------------------------------------
//=============================================================================
DatalinkServer::DatalinkServer(QObject * parent)
  : QObject(parent),
    retryBind(0),retryExt(0),
    m_readOnly(false),
    m_active(false),
    m_bindEnabled(true),
    m_heartbeatEnabled(true),
    m_name("user"),
    m_extctrEnabled(true)
{
  qApp->setProperty("DatalinkServer",qVariantFromValue(this));

  m_name=QSettings().value("serverName",m_name).toString().simplified();
  if(m_name.isEmpty()){
    foreach(QString s,QProcess::systemEnvironment()){
      if(!s.startsWith("USER"))continue;
      s=s.mid(s.indexOf('=')+1).trimmed();
      if(s.isEmpty())break;
      m_name=s;
      break;
    }
  }

  m_extctrEnabled=QSettings().value("extctrEnabled").toBool();
  masterHost=QSettings().value("masterHost").toString();
  if(!masterHost.isNull()){
    qDebug("%s: %s",tr("Linked to master GCU").toUtf8().data(),masterHost.toString().toUtf8().data());
  }

  bDontAutoConnectExt=false;

  server=new QTcpServer(this);
  extSocket=new QTcpSocket(this);
  extSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
  extSocket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
  connect(extSocket,SIGNAL(connected()),this,SLOT(extSocketConnected()));
  connect(extSocket,SIGNAL(readyRead()),this,SLOT(socketReadyRead()));
  connect(extSocket,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
  connect(extSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(socketError(QAbstractSocket::SocketError)));


  //heartbeat timer
  connect(&heartbeatTimer,SIGNAL(timeout()),this,SLOT(heartbeatTimeout()));
  heartbeatTimer.setInterval(1500);
  heartbeatTimer.start();

  //ext source connect
  connect(&connectExtTimer,SIGNAL(timeout()),this,SLOT(connectToAny()));
  connectExtTimer.setInterval(2000);
  if(!masterHost.isNull())connectToAny();

  //udp discover
  udpDiscover=new QUdpSocket(this);
  connect(udpDiscover,SIGNAL(readyRead()),this, SLOT(udpDiscoverRead()));
  tryBindUdpDiscover();

  udpDiscoverSearch=new QUdpSocket(this);
  uint search_port=UDP_PORT_DISCOVER+1;
  while((!udpDiscoverSearch->bind(QHostAddress::Any,search_port,QUdpSocket::DontShareAddress))&&(search_port<(UDP_PORT_DISCOVER+50)))
    search_port++;
  connect(udpDiscoverSearch,SIGNAL(readyRead()),this, SLOT(udpDiscoverSearchRead()));
}
//=============================================================================
void DatalinkServer::activate()
{
  setActive(true);
}
//=============================================================================
bool DatalinkServer::isPacketLocalForwardable(const QByteArray &ba) const
{
  _bus_packet *packet=(_bus_packet*)(ba.data());
  if(packet->id==idx_dlink) packet=(_bus_packet*)(ba.data()+bus_packet_size_hdr+sizeof(IDENT::_squawk));
  if(packet->id==idx_jsexec)return false;
  return true;
}
//=============================================================================
//=============================================================================
void DatalinkServer::dataSend(const QByteArray &ba)
{
  if(readOnly() || (!active()))return;
  if(!isPacketLocalForwardable(ba))return;
  sendUplinkLocal(ba);
  //forward to all other ext GCU clients
  forwardPacket(extSocket,ba);
}
//=============================================================================
void DatalinkServer::sendUplinkLocal(const QByteArray &ba)
{
  emit loacalDataSend(ba);
  //send through ext GCU server
  if(extConnected()){
    sendPacket(extSocket,ba);
  }
}
//=============================================================================
void DatalinkServer::localDataReceived(const QByteArray &ba)
{
  if(!active())return;
  emit dataReceived(ba);
  //check for no-forward packets
  if(isPacketLocalForwardable(ba))
    forwardPacket(NULL,ba);  //forward to all server clients
}
//=============================================================================
void DatalinkServer::forwardPacket(QTcpSocket *src_socket,const QByteArray &ba)
{
  if(!server->isListening())return;
  QByteArray tcpData=makeTcpPacket(ba);
  foreach(QTcpSocket *socket,connections.keys()){
    if(socket==src_socket)continue;
    if(socket==extSocket)continue;
    if(!connections[socket].datalink)continue;
    socket->write(tcpData);
  }
}
void DatalinkServer::sendPacket(QTcpSocket *socket,const QByteArray &ba)
{
  socket->write(makeTcpPacket(ba));
}
QByteArray DatalinkServer::makeTcpPacket(const QByteArray &ba) const
{
  quint16 sz=ba.size();
  quint16 crc16=CRC_16_IBM((uint8_t*)ba.data(),ba.size(),0xFFFF);
  QByteArray tcpData;
  tcpData.append((const char*)&sz,sizeof(sz));
  tcpData.append((const char*)&crc16,sizeof(crc16));
  tcpData.append(ba);
  return tcpData;
}
//=============================================================================
bool DatalinkServer::isLocalHost(const QHostAddress address) const
{
  foreach(const QHostAddress &a,QNetworkInterface::allAddresses())
    if(address==a) return true;
  return false;
}
//=============================================================================
void DatalinkServer::tryBindServer(void)
{
  if(!active())return;
  if((m_bindEnabled==false) || (!server->listen(QHostAddress::Any,TCP_PORT_SERVER))) {
    //server port is busy by another local GCU
    retryBind++;
    if(retryBind<=1 && m_bindEnabled){
      qWarning("%s: %s",tr("Unable to start server").toUtf8().data(),server->errorString().toUtf8().data());
    }
    uint to=1000+(retryBind/10)*1000;
    QTimer::singleShot(to>10000?10000:to,this,SLOT(tryBindServer()));
    //try to connect to local GCU server from another gcu
    if(extConnected()||bDontAutoConnectExt)return;
    if(extSocket && extSocket->state()!=QAbstractSocket::UnconnectedState)return;
    connectToServer(QHostAddress::LocalHost);
    return;
  }
  if(retryBind)qDebug("%s",tr("Server binded").toUtf8().data());
  retryBind=0;
  connect(server,SIGNAL(newConnection()),this,SLOT(newConnection()));
}
//=============================================================================
void DatalinkServer::connectToServer(const QHostAddress address)
{
  bDontAutoConnectExt=false;
  if(!active()){
    qWarning("%s",tr("Server inactive").toUtf8().data());
    return;
  }
  if(server->isListening() && address!=QHostAddress::LocalHost && isLocalHost(address)){
    qWarning("%s",QString("%1 (%2)").arg(tr("Can't connect to local server")).arg(address.toString()).toUtf8().data());
    return;
  }
  extSocket->abort();
  extSocket->connectToHost(address,TCP_PORT_SERVER);
}
void DatalinkServer::connectToServer(const QString server)
{
  QHostAddress address;
  QString sname;
  QString s=server.trimmed();
  if(s.isEmpty())return;
  if(s.contains('@')){
    sname=s.left(s.indexOf('@'));
    address=s.mid(s.indexOf('@')+1);
  }else{
    address=QHostAddress(s);
  }
  connectToServer(address);
}
void DatalinkServer::disconnect()
{
  extSocket->abort();
  bDontAutoConnectExt=true;
}
//=============================================================================
const QString DatalinkServer::connectedServer(void) const
{
  return extConnected()?extServerName:QString();
}
void DatalinkServer::tryExtReconnect()
{
  if(extConnected())return;
}
//=============================================================================
void DatalinkServer::connectToAny(void)
{
  uint to=1000+(retryExt/10)*1000;
  connectExtTimer.start(to>5000?5000:to);
  if(!active()) return;
  if(bDontAutoConnectExt) return;

  if(extConnected())return;
  if(extSocket && extSocket->state()!=QAbstractSocket::UnconnectedState)return;
  //try to connect ext GCU server
  if(!masterHost.isNull()){
    connectToServer(masterHost);
  }else if(!lastExtServerConnected.isNull()){
    if(isLocalHost(lastExtServerConnected)&&server->isListening()){
      for(QHash<QHostAddress,QString>::iterator it=servers.begin();it!=servers.end();){
        if(isLocalHost(it.key()))it=servers.erase(it);
        else ++it;
      }
      lastExtServerConnected=QHostAddress();
      return;
    }
    connectToServer(lastExtServerConnected);
    //if(!servers.isEmpty()) lastExtServerConnected=QHostAddress();
  }else if(!servers.isEmpty()){
    connectToServer(servers.keys().at(retryExt%servers.keys().size()));
  }
  retryExt++;
  //qDebug()<<"connectToAny";
}
//=============================================================================
//=============================================================================
void DatalinkServer::newConnection()
{
  while(server->hasPendingConnections()){
    QTcpSocket *socket=server->nextPendingConnection();
    if(!active()){
      socket->disconnectFromHost();
      continue;
    }
    connections[socket].datalink=false;
    connections[socket].local=isLocalHost(socket->peerAddress());
    connections[socket].size=0;
    connections[socket].hdr.clear();
    socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
    socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    connect(socket,SIGNAL(readyRead()),this,SLOT(socketReadyRead()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(socketError(QAbstractSocket::SocketError)));
  }
}
//=============================================================================
void DatalinkServer::socketReadyRead()
{
  //read uplink from conected clients
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!(connections.contains(socket) && active())){
    socket->disconnectFromHost();
    return;
  }
  //HTTP socket
  if(!connections[socket].datalink){
    httpRequest(socket);
    if(!connections[socket].datalink)return;
  }
  //DATALINK socket
  while(socket->bytesAvailable()){
    quint16 sz=connections[socket].size;
    quint16 crc16=connections[socket].crc16;
    if(sz==0){
      if(socket->bytesAvailable()<(int)(sizeof(sz)+sizeof(crc16)))
        return;
      socket->read((char*)&sz,sizeof(sz));
      socket->read((char*)&crc16,sizeof(crc16));
      if(sz>2048){
        qWarning("tcp sz error: %s (%u)",socket->peerAddress().toString().toUtf8().data(),sz);
        socket->disconnectFromHost();
        return;
      }
      connections[socket].size=sz;
      connections[socket].crc16=crc16;
    }
    if(socket->bytesAvailable()<(int)sz)
      return;
    connections[socket].size=0;
    QByteArray ba=socket->read(sz);
    if(crc16!=CRC_16_IBM((uint8_t*)ba.data(),ba.size(),0xFFFF)){
      qWarning("tcp crc error: %s:%u",socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
      qDebug()<<sz<<ba.toHex().toUpper();
      socket->disconnectFromHost();
      return;
    }
    if(socket==extSocket){
      if(extConnected()){
        //received from ext GCU server
        //qDebug()<<sz<<ba.toHex().toUpper();
        emit dataReceived(ba); //read data by GCU
        forwardPacket(socket,ba); //forward to all other clients
      }
    }else if(extctrEnabled()||connections[socket].local){
      //received from ext GCU clients
      //qDebug()<<sz<<ba.toHex().toUpper();
      //_bus_packet *packet=(_bus_packet*)(ba.data());
      //if(packet->id==0 && packet->srv.cmd==apc_loader) qDebug()<<"tx"<<ba.toHex();
      if((uint8_t)ba.at(0)!=idx_downstream) sendUplinkLocal(ba);
      emit dataReceived(ba); //read data by GCU
      forwardPacket(socket,ba); //forward to all other clients
    }
  }
}
//=============================================================================
void DatalinkServer::httpRequest(QTcpSocket *socket)
{
  if(!socket->canReadLine())return;
  bool reqDone=false;
  while(socket->canReadLine()){
    QString line=socket->readLine();
    //qDebug()<<"line:"<<line.trimmed();
    //qDebug()<<"line:"<<QByteArray(line.toUtf8()).toHex().toUpper();
    if(!line.trimmed().isEmpty()){
      connections[socket].hdr.append(line.trimmed());
    }else{
      reqDone=true;
      break;
    }
  }
  if(!reqDone)return;
  //header received
  const QStringList &hdr=connections[socket].hdr;
  QHash<QString,QString> hash; //name:value, name lowercase
  foreach(QString s,hdr){
    if(s.contains(':'))
      hash.insert(s.left(s.indexOf(':')).trimmed().toLower(),s.mid(s.indexOf(':')+1).trimmed());
  }
  //qDebug()<<"hdr:"<<hdr;
  while(1){
    if(hdr.isEmpty())break;

    if(socket==extSocket){
      //response for GET /datalink
      if(!hdr.at(0).contains("200 OK",Qt::CaseInsensitive)){
        qDebug()<<hdr.at(0);
        break;
      }
      if(!hash.value("content-type").contains("application/octet-stream",Qt::CaseInsensitive)){
        qDebug()<<hash.value("content-type");
        break;
      }
      lastExtServerConnected=socket->peerAddress();
      retryExt=0;
      extServerName=socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
      if(hash.contains("server"))
        extServerName.prepend(hash.value("server")+"@");
      connections[socket].datalink=true;
      connections[socket].size=0;
      connections[socket].crc16=0;
      qDebug("#%s: %s",tr("server").toUtf8().data(),connectedServer().toUtf8().data());
      emit extConnectedChanged(extConnected());
      return;
    }
    //request for service
    QStringList rlist=hdr.at(0).simplified().split(' ');
    if(rlist.size()<2)break;
    if(rlist.at(0)=="GET"){
      QString req=QUrl::fromPercentEncoding(rlist.at(1).toUtf8());
      QTextStream stream(socket);
      stream.setAutoDetectUnicode(true);
      if(req=="/datalink"){
        stream << "HTTP/1.0 200 OK\r\n";
        stream << "Content-Type: application/octet-stream\r\n";
        stream << QString("Server: %1\r\n").arg(name());
        stream << "\r\n";
        stream.flush();
        QString sname=socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
        if(hash.contains("from"))
          sname.prepend(hash.value("from")+"@");
        if(socket->peerAddress()==extSocket->peerAddress()){
          qWarning("%s",tr("Client connection refused").toUtf8().data());
          break;
        }
        connections[socket].datalink=true;
        connections[socket].size=0;
        connections[socket].crc16=0;
        if(connections.size()>1)sname.append(QString(" [%1]").arg(connections.size()));
        qDebug("#%s: %s",tr("client").toUtf8().data(),sname.toUtf8().data());
        if(!extctrEnabled()){
          if(connections[socket].local)qDebug("%s",tr("Local client controls enabled").toUtf8().data());
          else qDebug("%s",tr("External client controls disabled").toUtf8().data());
        }
        return;
      }else{
        bool ok=false;
        emit httpRequest(stream,req,&ok);
        if(!ok){
          stream << "HTTP/1.1 404 Not Found\r\n";
          stream << "Content-Type: text/html; charset=\"utf-8\"\r\n";
          stream << "\r\n";
          stream << QString("<b>GCU HTTP Server</b> (%1:%2)").arg(server->serverAddress().toString()).arg(server->serverPort());
          if(req!="/") stream << QString("<br>No service for '%1'").arg(req);
          stream << "<hr size=1>";
          stream << QString("<a href=%1>%1</a> - %2<br>").arg("/kml").arg("Google Earth KML");
          stream << QString("<a href=%1>%1</a> - %2<br>").arg("/datalink").arg("Datalink stream [uint16 packet size][CRC_16_IBM][packet data]");
          stream << QString("<a href=%1>%1</a> - %2<br>").arg("/mandala").arg("Mandala XML data and commands");
          stream << QString("<br>More info here: <a href=%1>%1</a>").arg("http://docs.uavos.com/sw/comm");
        }
      }
      socket->close();
      return;
    }
    break;
  }//while
  //error
  socket->close();
  //socket->disconnectFromHost();
}
//=============================================================================
void DatalinkServer::socketDisconnected()
{
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!socket)return;
  connections.remove(socket);
  if(socket!=extSocket){
    socket->deleteLater();
  }else{
    emit extConnectedChanged(extConnected());
    if(!lastExtServerConnected.isNull())
      connectExtTimer.start(2000);
  }
}
//=============================================================================
void DatalinkServer::socketError(QAbstractSocket::SocketError socketError)
{
  Q_UNUSED(socketError)
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!socket)return;
  if(connections.contains(socket) && connections[socket].datalink)
    qDebug("#%s (%s:%u).",socket->errorString().toUtf8().data(),socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
  socket->abort();
  connections.remove(socket);
  if(socket!=extSocket){
    socket->deleteLater();
  }else{
    emit extConnectedChanged(false);
  }
}
//=============================================================================
//=============================================================================
void DatalinkServer::extSocketConnected()
{
  retryExt=0;
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!active()){
    socket->disconnectFromHost();
    return;
  }
  connections[socket].size=0;
  connections[socket].datalink=false;
  connections[socket].local=isLocalHost(socket->peerAddress());
  connections[socket].hdr.clear();
  QTextStream stream(socket);
  stream << "GET /datalink HTTP/1.0\r\n";
  stream << QString("From: %1\r\n").arg(name());
  stream << "\r\n";
  stream.flush();
}
//=============================================================================
//=============================================================================
void DatalinkServer::tryBindUdpDiscover(void)
{
  if(!udpDiscover->bind(QHostAddress::Any,UDP_PORT_DISCOVER))
    QTimer::singleShot(5000, this, SLOT(tryBindUdpDiscover()));
}
//=============================================================================
void DatalinkServer::udpDiscoverRead(void)
{
  while(udpDiscover->hasPendingDatagrams()) {
    QHostAddress srcHost;
    quint16 srcPort;
    QByteArray datagram;
    datagram.resize(udpDiscover->pendingDatagramSize());
    udpDiscover->readDatagram(datagram.data(),datagram.size(),&srcHost,&srcPort);
    //qDebug()<<datagram;
    if(datagram==QByteArray("search.gcu.uavos.com")){
      if(!server->isListening())continue;
      udpDiscover->writeDatagram(QByteArray(QString("server.gcu.uavos.com:%1").arg(name()).toUtf8()),srcHost,srcPort);
      continue;
    }
  }
}
//=============================================================================
void DatalinkServer::udpDiscoverSearchRead(void)
{
  while(udpDiscoverSearch->hasPendingDatagrams()) {
    QHostAddress srcHost;
    quint16 srcPort;
    QByteArray datagram;
    datagram.resize(udpDiscoverSearch->pendingDatagramSize());
    udpDiscoverSearch->readDatagram(datagram.data(),datagram.size(),&srcHost,&srcPort);
    //qDebug()<<datagram;
    if(datagram.startsWith(QByteArray("server.gcu.uavos.com:"))){
      if(server->isListening() && isLocalHost(srcHost))continue;
      QString uname=QString(datagram);
      uname.remove(0,uname.indexOf(':')+1);
      if(!servers.contains(srcHost)){
        QString sname=QString("%1@%2").arg(uname).arg(srcHost.toString());
        qDebug("#%s: %s",tr("found server").toUtf8().data(),sname.toUtf8().data());
        servers[srcHost]=uname;
        emit serverDiscovered(srcHost,sname);
      }else servers[srcHost]=uname;
      continue;
    }
  }
}
//=============================================================================
void DatalinkServer::discover(void)
{
  if(!active())return;
  if(udpDiscoverSearch->state()==QAbstractSocket::BoundState)
    udpDiscoverSearch->writeDatagram(QByteArray("search.gcu.uavos.com"),QHostAddress::Broadcast,UDP_PORT_DISCOVER);
  QTimer::singleShot(5555, this, SLOT(discover()));
  //qDebug()<<"discover";
}
//=============================================================================
//=============================================================================
void DatalinkServer::heartbeatTimeout(void)
{
  if(readOnly() || extConnected() || (!active()))return;
  emit heartbeat(QByteArray().append((char)idx_ping).append((char)0));
}
//=============================================================================
//=============================================================================
// PROPERTIES
//=============================================================================
bool DatalinkServer::readOnly() const
{
  return m_readOnly;
}
void DatalinkServer::setReadOnly(bool v)
{
  if(m_readOnly==v)return;
  m_readOnly=v;
  emit readOnlyChanged(v);
}
//----------------------------------------------
bool DatalinkServer::active() const
{
  return m_active;
}
void DatalinkServer::setActive(bool v)
{
  if(m_active==v)return;
  m_active=v;
  if(v){
    qDebug("%s",tr("Datalink server activated").toUtf8().data());
    retryExt=0;
    retryBind=0;
    tryBindServer();
    discover();
  }else{
    foreach(QTcpSocket *socket,connections.keys()){
      socket->disconnectFromHost();
    }
    server->close();
    qDebug("%s",tr("Datalink server inactive").toUtf8().data());
  }
  emit activeChanged(v);
}
//----------------------------------------------
bool DatalinkServer::bindEnabled() const
{
  return m_bindEnabled;
}
void DatalinkServer::setBindEnabled(bool v)
{
  if(m_bindEnabled==v)return;
  m_bindEnabled=v;
  if(v){
    retryBind=0;
    tryBindServer();
  }
  emit bindEnabledChanged(v);
}
//----------------------------------------------
bool DatalinkServer::heartbeatEnabled() const
{
  return m_heartbeatEnabled;
}
void DatalinkServer::setHeartbeatEnabled(bool v)
{
  if(m_heartbeatEnabled==v)return;
  m_heartbeatEnabled=v;
  if(v){
    heartbeatTimer.start();
  }else{
    heartbeatTimer.stop();
  }
  emit heartbeatEnabledChanged(v);
}
//----------------------------------------------
QString DatalinkServer::name() const
{
  return m_name;
}
void DatalinkServer::setName(QString v)
{
  if(m_name==v)return;
  m_name=v;
  emit nameChanged(v);
}
//----------------------------------------------
bool DatalinkServer::extctrEnabled() const
{
  return m_extctrEnabled;
}
void DatalinkServer::setExtctrEnabled(bool v)
{
  if(m_extctrEnabled==v)return;
  m_extctrEnabled=v;
  QSettings().setValue("extctrEnabled",v);
  emit extctrEnabledChanged(v);
}
//----------------------------------------------
QStringList DatalinkServer::serverNames()
{
  QStringList st;
  foreach(QHostAddress host,servers.keys()){
    st.append(servers.value(host)+"@"+host.toString());
  }
  return st;
}
//----------------------------------------------
bool DatalinkServer::extConnected(void) const
{
  return connections.contains(extSocket) && extSocket->state()==QAbstractSocket::ConnectedState && connections[extSocket].datalink;
}
//=============================================================================
//=============================================================================
