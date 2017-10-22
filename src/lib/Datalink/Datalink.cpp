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
#include "Datalink.h"
#include "tcp_ports.h"
#include <QtNetwork>
#include "MandalaCore.h"
#include "node.h"
#include "crc.h"
//-----------------------------------------------------------------------------
//=============================================================================
Datalink::Datalink(FactSystem *parent)
  : DatalinkFacts(parent),
    retryBind(0)
{
  connect(f_active,&Fact::valueChanged,this,&Datalink::activeChanged);
  connect(f_hbeat,&Fact::valueChanged,this,&Datalink::hbeatChanged);

  tcpServer=new QTcpServer(this);

  //heartbeat timer
  connect(&heartbeatTimer,SIGNAL(timeout()),this,SLOT(heartbeatTimeout()));
  heartbeatTimer.setInterval(1500);
  heartbeatTimer.start();
}
//=============================================================================
void Datalink::dataSend(const QByteArray &ba)
{
  if(f_readonly->value().toBool() || (!f_active->value().toBool()))return;
  //if(!isPacketLocalForwardable(ba))return;
  sendUplinkLocal(ba);
  //forward to all other ext GCU clients
  //forwardPacket(extSocket,ba);
}
//=============================================================================
void Datalink::sendUplinkLocal(const QByteArray &ba)
{
  emit loacalDataSend(ba);
  //send through ext GCU server
  /*if(extConnected()){
    sendPacket(extSocket,ba);
  }*/
}
//=============================================================================
void Datalink::localDataReceived(const QByteArray &ba)
{
  if(!f_active->value().toBool())return;
  emit dataReceived(ba);
  //check for no-forward packets
  //if(isPacketLocalForwardable(ba))
    forwardPacket(NULL,ba);  //forward to all server clients
}
//=============================================================================
void Datalink::forwardPacket(QTcpSocket *src_socket,const QByteArray &ba)
{
  if(!tcpServer->isListening())return;
  QByteArray tcpData=makeTcpPacket(ba);
  foreach(QTcpSocket *socket,connections.keys()){
    if(socket==src_socket)continue;
    //if(socket==extSocket)continue;
    if(!connections[socket].datalink)continue;
    socket->write(tcpData);
  }
}
void Datalink::sendPacket(QTcpSocket *socket,const QByteArray &ba)
{
  socket->write(makeTcpPacket(ba));
}
QByteArray Datalink::makeTcpPacket(const QByteArray &ba) const
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
void Datalink::tryBind(void)
{
  if(!f_active->value().toBool())return;
  if(!tcpServer->listen(QHostAddress::Any,TCP_PORT_SERVER)) {
    //server port is busy by another local GCU
    retryBind++;
    if(retryBind<=1){
      qWarning("%s: %s",tr("Unable to start server").toUtf8().data(),tcpServer->errorString().toUtf8().data());
    }
    uint to=1000+(retryBind/10)*1000;
    QTimer::singleShot(to>10000?10000:to,this,SLOT(tryBind()));
    return;
  }
  if(retryBind)qDebug("%s",tr("Server binded").toUtf8().data());
  retryBind=0;
  connect(tcpServer,SIGNAL(newConnection()),this,SLOT(newConnection()));
}
//=============================================================================
//=============================================================================
void Datalink::newConnection()
{
  while(tcpServer->hasPendingConnections()){
    QTcpSocket *socket=tcpServer->nextPendingConnection();
    if(!f_active->value().toBool()){
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
void Datalink::socketReadyRead()
{
  //read uplink from conected clients
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!(connections.contains(socket) && f_active->value().toBool())){
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
    if(f_extctr->value().toBool()||connections[socket].local){
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
void Datalink::httpRequest(QTcpSocket *socket)
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
        /*if(socket->peerAddress()==extSocket->peerAddress()){
          qWarning("%s",tr("Client connection refused").toUtf8().data());
          break;
        }*/
        connections[socket].datalink=true;
        connections[socket].size=0;
        connections[socket].crc16=0;
        if(connections.size()>1)sname.append(QString(" [%1]").arg(connections.size()));
        qDebug("#%s: %s",tr("client").toUtf8().data(),sname.toUtf8().data());
        if(!f_extctr->value().toBool()){
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
          stream << QString("<b>GCU HTTP Server</b> (%1:%2)").arg(tcpServer->serverAddress().toString()).arg(tcpServer->serverPort());
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
void Datalink::socketDisconnected()
{
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!socket)return;
  connections.remove(socket);
  socket->deleteLater();
}
//=============================================================================
void Datalink::socketError(QAbstractSocket::SocketError socketError)
{
  Q_UNUSED(socketError)
  QTcpSocket *socket=qobject_cast<QTcpSocket*>(sender());
  if(!socket)return;
  if(connections.contains(socket) && connections[socket].datalink)
    qDebug("#%s (%s:%u).",socket->errorString().toUtf8().data(),socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
  socket->abort();
  connections.remove(socket);
  socket->deleteLater();
}
//=============================================================================
void Datalink::heartbeatTimeout(void)
{
  if(f_readonly->value().toBool() || (!f_active->value().toBool()))return;
  emit heartbeat(QByteArray().append((char)idx_ping).append((char)0));
}
//=============================================================================
//=============================================================================
// PROPERTIES
//=============================================================================
void Datalink::activeChanged()
{
  if(f_active->value().toBool()){
    retryBind=0;
    tryBind();
  }else{
    foreach(QTcpSocket *socket,connections.keys()){
      socket->disconnectFromHost();
    }
    tcpServer->close();
  }
}
//----------------------------------------------
void Datalink::hbeatChanged()
{
  if(f_hbeat->value().toBool()){
    heartbeatTimer.start();
  }else{
    heartbeatTimer.stop();
  }
}
//----------------------------------------------
//=============================================================================
