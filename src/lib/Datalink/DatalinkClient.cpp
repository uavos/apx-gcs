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
#include "DatalinkClient.h"
#include "DatalinkFacts.h"
#include "crc.h"
//-----------------------------------------------------------------------------
//=============================================================================
DatalinkClient::DatalinkClient(DatalinkFacts *datalink, QTcpSocket *socket)
  : Fact(datalink->f_clients,"client#","","",FactItem,NoData),
    datalink(datalink),
    socket(socket)
{
  socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
  socket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
  connect(socket,&QTcpSocket::connected,this,&DatalinkClient::socketConnected);
  connect(socket,&QTcpSocket::readyRead,this,&DatalinkClient::socketReadyRead);
  connect(socket,&QTcpSocket::disconnected,this,&DatalinkClient::socketDisconnected);
  connect(socket,static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),this,&DatalinkClient::socketError);
}
//=============================================================================
//=============================================================================
void DatalinkClient::socketConnected()
{
  clientData.size=0;
  clientData.datalink=false;
  clientData.hdr.clear();
  QTextStream stream(socket);
  stream << "GET /datalink HTTP/1.0\r\n";
  stream << QString("From: %1\r\n").arg(datalink->f_name->text());
  stream << "\r\n";
  stream.flush();
}
//=============================================================================
void DatalinkClient::socketReadyRead()
{
  //HTTP socket
  if(!clientData.datalink){
    httpRequest();
    if(!clientData.datalink)return;
  }
  //DATALINK socket
  while(socket->bytesAvailable()){
    quint16 sz=clientData.size;
    quint16 crc16=clientData.crc16;
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
      clientData.size=sz;
      clientData.crc16=crc16;
    }
    if(socket->bytesAvailable()<(int)sz)
      return;
    clientData.size=0;
    QByteArray ba=socket->read(sz);
    if(crc16!=CRC_16_IBM((uint8_t*)ba.data(),ba.size(),0xFFFF)){
      qWarning("tcp crc error: %s:%u",socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
      qDebug()<<sz<<ba.toHex().toUpper();
      socket->disconnectFromHost();
      return;
    }
    emit dataReceived(ba);
  }
}
//=============================================================================
void DatalinkClient::httpRequest()
{
  if(!socket->canReadLine())return;
  bool reqDone=false;
  while(socket->canReadLine()){
    QString line=socket->readLine();
    //qDebug()<<"line:"<<line.trimmed();
    //qDebug()<<"line:"<<QByteArray(line.toUtf8()).toHex().toUpper();
    if(!line.trimmed().isEmpty()){
      clientData.hdr.append(line.trimmed());
    }else{
      reqDone=true;
      break;
    }
  }
  if(!reqDone)return;
  //header received
  const QStringList &hdr=clientData.hdr;
  QHash<QString,QString> hash; //name:value, name lowercase
  foreach(QString s,hdr){
    if(s.contains(':'))
      hash.insert(s.left(s.indexOf(':')).trimmed().toLower(),s.mid(s.indexOf(':')+1).trimmed());
  }
  //qDebug()<<"hdr:"<<hdr;
  while(1){
    if(hdr.isEmpty())break;

    //response for GET /datalink
    if(!hdr.at(0).contains("200 OK",Qt::CaseInsensitive)){
      qDebug()<<hdr.at(0);
      break;
    }
    if(!hash.value("content-type").contains("application/octet-stream",Qt::CaseInsensitive)){
      qDebug()<<hash.value("content-type");
      break;
    }
    serverName=socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
    if(hash.contains("server"))
      serverName.prepend(hash.value("server")+"@");
    clientData.datalink=true;
    clientData.size=0;
    clientData.crc16=0;
    qDebug("#%s: %s",tr("server").toUtf8().data(),serverName.toUtf8().data());
    emit datalinkConnected();
    return;
  }//while
  //error
  socket->close();
  //socket->disconnectFromHost();
}
//=============================================================================
void DatalinkClient::socketDisconnected()
{
  emit disconnected();
  socket->deleteLater();
}
//=============================================================================
void DatalinkClient::socketError(QAbstractSocket::SocketError socketError)
{
  Q_UNUSED(socketError)
  if(clientData.datalink)
    qDebug("#%s (%s:%u).",socket->errorString().toUtf8().data(),socket->peerAddress().toString().toUtf8().data(),socket->peerPort());
  socket->abort();
  socket->deleteLater();
}
//=============================================================================
//=============================================================================
QByteArray DatalinkClient::makeTcpPacket(const QByteArray &ba)
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
void DatalinkClient::sendPacket(const QByteArray &ba)
{
  if(!clientData.datalink)return;
  socket->write(makeTcpPacket(ba));
}
//=============================================================================
