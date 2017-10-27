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
#include "DatalinkSocket.h"
#include "Datalink.h"
#include "crc.h"
//-----------------------------------------------------------------------------
//=============================================================================
DatalinkSocket::DatalinkSocket(Fact *parent, QString title, QTcpSocket *socket, bool bServer,QString serverName)
  : Fact(parent,"socket#",title,"",FactItem,NoData),
    socket(socket),
    bServer(bServer),serverName(serverName)
{
  m_connected=socket->isOpen();
  socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
  socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);

  data.size=0;
  data.crc16=0;
  data.datalink=false;

  connect(socket,&QTcpSocket::readyRead,this,&DatalinkSocket::socketReadyRead);
  connect(socket,&QTcpSocket::disconnected,this,&DatalinkSocket::socketDisconnected);
  connect(socket,&QTcpSocket::disconnected,this,&DatalinkSocket::disconnected);
  connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,&DatalinkSocket::socketError);
  connect(socket,&QTcpSocket::stateChanged,this,&DatalinkSocket::socketStateChanged);

  if(!bServer){
    connect(socket,&QTcpSocket::connected,this,&DatalinkSocket::socketClientConnected);
  }
}
//=============================================================================
bool DatalinkSocket::connected() const
{
  return m_connected;
}
void DatalinkSocket::setConnected(const bool &v)
{
  if(m_connected==v)return;
  m_connected=v;
  emit connectedChanged();
}
//=============================================================================
void DatalinkSocket::disconnectSocket()
{
  socket->abort();
  socket->close();
  socketDisconnected();
}
//=============================================================================
void DatalinkSocket::socketClientConnected()
{
  setConnected(true);
  data.size=0;
  data.datalink=false;
  data.hdr.clear();
  QTextStream stream(socket);
  stream << "GET /datalink HTTP/1.0\r\n";
  stream << QString("From: %1\r\n").arg(serverName);
  stream << "\r\n";
  stream.flush();
}
//=============================================================================
void DatalinkSocket::socketReadyRead()
{
  if(!readHeader())return;
  if(bServer){
    if(!checkDatalinkRequestHeader())return;
  }else{
    if(!checkDatalinkResponseHeader())return;
  }
  QObject::disconnect(socket,&QTcpSocket::readyRead,this,&DatalinkSocket::socketReadyRead);
  connect(socket,&QTcpSocket::readyRead,this,&DatalinkSocket::readDatalinkData);
  readDatalinkData();
}
//=============================================================================
void DatalinkSocket::socketDisconnected()
{
  setConnected(false);
  //emit disconnected();
  if(bServer){
    socket->deleteLater();
  }else{
    data.size=0;
    data.crc16=0;
    data.datalink=false;
    QObject::disconnect(socket,&QTcpSocket::readyRead,this,&DatalinkSocket::socketReadyRead);
    QObject::disconnect(socket,&QTcpSocket::readyRead,this,&DatalinkSocket::readDatalinkData);
    connect(socket,&QTcpSocket::readyRead,this,&DatalinkSocket::socketReadyRead);
  }
}
//=============================================================================
void DatalinkSocket::socketError(QAbstractSocket::SocketError socketError)
{
  Q_UNUSED(socketError)
  if(data.datalink)
    qDebug("#%s (%s:%u).",socket->errorString().toUtf8().data(),socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
  setStatus("Error");
  disconnectSocket();
}
//=============================================================================
bool DatalinkSocket::readHeader()
{
  if(data.datalink)return true;
  if(!socket->canReadLine())return false;
  bool reqDone=false;
  while(socket->canReadLine()){
    QString line=socket->readLine();
    //qDebug()<<"line:"<<line.trimmed();
    //qDebug()<<"line:"<<QByteArray(line.toUtf8()).toHex().toUpper();
    if(!line.trimmed().isEmpty()){
      data.hdr.append(line.trimmed());
    }else{
      reqDone=true;
      break;
    }
  }
  if(!reqDone)return false;
  //header received
  const QStringList &hdr=data.hdr;
  foreach(QString s,hdr){
    if(s.contains(':'))
      data.hdr_hash.insert(s.left(s.indexOf(':')).trimmed().toLower(),s.mid(s.indexOf(':')+1).trimmed());
  }
  //qDebug()<<"hdr:"<<hdr;
  if(!data.hdr.isEmpty())return true;
  //error
  socket->close();
  return false;
}
//=============================================================================
bool DatalinkSocket::checkDatalinkRequestHeader()
{
  if(data.datalink)return true;
  //request for service
  while(1){
    QStringList rlist=data.hdr.at(0).simplified().split(' ');
    if(rlist.size()<2)break;
    if(rlist.at(0)=="GET"){
      QString req=QUrl::fromPercentEncoding(rlist.at(1).toUtf8());
      QTextStream stream(socket);
      stream.setAutoDetectUnicode(true);
      if(req=="/datalink"){
        stream << "HTTP/1.0 200 OK\r\n";
        stream << "Content-Type: application/octet-stream\r\n";
        stream << QString("Server: %1\r\n").arg(serverName);
        stream << "\r\n";
        stream.flush();
        QString sname;
        if(isLocalHost(socket->peerAddress()))sname="localhost";
        else sname=socket->peerAddress().toString();
        //sname+=":"+QString::number(socket->peerPort());
        if(data.hdr_hash.contains("from"))
          sname.prepend(data.hdr_hash.value("from")+"@");
        /*if(socket->peerAddress()==extSocket->peerAddress()){
          qWarning("%s",tr("Client connection refused").toUtf8().data());
          break;
        }*/
        data.datalink=true;
        data.size=0;
        data.crc16=0;
        /*if(connections.size()>1)sname.append(QString(" [%1]").arg(connections.size()));
        qDebug("#%s: %s",tr("client").toUtf8().data(),sname.toUtf8().data());
        if(!extctrEnabled()){
          if(data.local)qDebug("%s",tr("Local client controls enabled").toUtf8().data());
          else qDebug("%s",tr("External client controls disabled").toUtf8().data());
        }*/
        qDebug("#client: %s",sname.toUtf8().data());
        setTitle(sname);
        setStatus("Datalink");
        emit datalinkConnected();
        return true;
      }else{
        bool ok=false;
        setStatus("HTTP");
        emit httpRequest(stream,req,&ok);
        if(!ok){
          stream << "HTTP/1.1 404 Not Found\r\n";
          stream << "Content-Type: text/html; charset=\"utf-8\"\r\n";
          stream << "\r\n";
          stream << QString("<b>GCS HTTP Server</b> (%1:%2)").arg(socket->localAddress().toString()).arg(socket->localPort());
          if(req!="/") stream << QString("<br>No service for '%1'").arg(req);
          stream << "<hr size=1>";
          stream << QString("<a href=%1>%1</a> - %2<br>").arg("/kml").arg("Google Earth KML");
          stream << QString("<a href=%1>%1</a> - %2<br>").arg("/datalink").arg("Datalink stream [uint16 packet size][CRC_16_IBM][packet data]");
          stream << QString("<a href=%1>%1</a> - %2<br>").arg("/mandala").arg("Mandala XML data and commands");
          stream << QString("<br>More info here: <a href=%1>%1</a>").arg("http://docs.uavos.com/sw/comm");
        }
      }
      socket->close();
      return false;
    }
  }
  //error
  socket->close();
  return false;
}
//=============================================================================
bool DatalinkSocket::checkDatalinkResponseHeader()
{
  if(data.datalink)return true;
  //response for GET /datalink
  while(1){
    if(!data.hdr.at(0).contains("200 OK",Qt::CaseInsensitive)){
      qDebug()<<data.hdr.at(0);
      break;
    }
    if(!data.hdr_hash.value("content-type").contains("application/octet-stream",Qt::CaseInsensitive)){
      qDebug()<<data.hdr_hash.value("content-type");
      break;
    }
    QString sname=socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
    if(data.hdr_hash.contains("server"))
      sname.prepend(data.hdr_hash.value("server")+"@");
    data.datalink=true;
    data.size=0;
    data.crc16=0;
    qDebug("#server connected: %s",sname.toUtf8().data());
    setStatus("Datalink");
    emit datalinkConnected();
    return true;
  }
  //error
  socket->close();
  return false;
}
//=============================================================================
//=============================================================================
void DatalinkSocket::readDatalinkData()
{
  while(socket->bytesAvailable()){
    quint16 sz=data.size;
    quint16 crc16=data.crc16;
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
      data.size=sz;
      data.crc16=crc16;
    }
    if(socket->bytesAvailable()<(int)sz)
      return;
    data.size=0;
    QByteArray ba=socket->read(sz);
    if(crc16!=CRC_16_IBM((uint8_t*)ba.data(),ba.size(),0xFFFF)){
      qWarning("tcp crc error: %s:%u",socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
      qDebug()<<sz<<ba.toHex().toUpper();
      socket->disconnectFromHost();
      return;
    }
    emit packetReceived(ba);
  }
}
//=============================================================================
QByteArray DatalinkSocket::makeTcpPacket(const QByteArray &ba) const
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
void DatalinkSocket::sendPacket(const QByteArray &ba)
{
  if(!data.datalink)return;
  if(!socket->isOpen())return;
  socket->write(makeTcpPacket(ba));
}
//=============================================================================
//=============================================================================
bool DatalinkSocket::isLocalHost(const QHostAddress address)
{
  //qDebug()<<QNetworkInterface::allAddresses();
  if(address.isLoopback())return true;
  foreach(const QHostAddress &a,QNetworkInterface::allAddresses())
    if(address.isEqual(a)) return true;
  return false;
}
//=============================================================================
void DatalinkSocket::socketStateChanged(QAbstractSocket::SocketState socketState)
{
  QString s;
  switch(socketState){
    default:
    case QAbstractSocket::UnconnectedState: break;
    case QAbstractSocket::HostLookupState:  s="Lookup"; break;
    case QAbstractSocket::ConnectingState:  s="Connecting"; break;
    case QAbstractSocket::ConnectedState:   s="Connected"; break;
    case QAbstractSocket::BoundState:       s="Bound"; break;
    case QAbstractSocket::ClosingState:     s="Closing"; break;
    case QAbstractSocket::ListeningState:   s="Listening"; break;
  }
  setStatus(s);
}
//=============================================================================
//=============================================================================

