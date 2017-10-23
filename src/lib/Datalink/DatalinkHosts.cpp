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
#include "DatalinkHosts.h"
#include "DatalinkHost.h"
#include "tcp_ports.h"
//=============================================================================
DatalinkHosts::DatalinkHosts(Datalink *parent)
  : Fact(parent,"hosts",tr("Remote servers"),tr("Discovered remote hosts"),GroupItem,ConstData)
{
  setFlatModel(true);

  f_datalink=parent;

  f_alloff=new Fact(this,"alloff",tr("Disconnect all"),tr("Drop all remote server connections"),FactItem,NoData);

  f_list=new Fact(this,"list",tr("Servers list"),tr("Found servers"),SectionItem,ConstData);
  bindValue(f_list);
  connect(f_list,&Fact::structChanged,this,&DatalinkHosts::updateStats);

  //udp discover
  udpReader=new QUdpSocket(this);
  connect(udpReader,&QUdpSocket::readyRead,this, &DatalinkHosts::udpRead);
  tryBind();

  udpAnnounce=new QUdpSocket(this);
  announceTimer.setInterval(20000);
  connect(&announceTimer,&QTimer::timeout,this,&DatalinkHosts::announce);
  connect(f_datalink->f_binded,&Fact::valueChanged,this,&DatalinkHosts::serverBindedChanged);

  updateStats();
}
//=============================================================================
void DatalinkHosts::tryBind(void)
{
  if(udpReader->bind(QHostAddress::Any,UDP_PORT_DISCOVER)){
    //qDebug()<<"udp discover binded";
    return;
  }
  QTimer::singleShot(5000, this, SLOT(tryBind()));
  //qWarning()<<"udp discover bind retry"<<udpReader->errorString();
}
//=============================================================================
void DatalinkHosts::udpRead(void)
{
  while(udpReader->hasPendingDatagrams()) {
    QHostAddress srcHost;
    quint16 srcPort;
    QByteArray datagram;
    datagram.resize(udpReader->pendingDatagramSize());
    udpReader->readDatagram(datagram.data(),datagram.size(),&srcHost,&srcPort);
    //qDebug()<<datagram;
    if(datagram.endsWith(QByteArray("@server.gcs.uavos.com"))){
      //if(f_datalink->f_binded->value().toBool() && DatalinkSocket::isLocalHost(srcHost))continue;
      const QString &uname=QString(datagram).left(datagram.indexOf('@'));
      //try to find and update existing
      DatalinkHost *host=NULL;
      foreach (FactTree *i , f_list->childItems()) {
        DatalinkHost *h=static_cast<DatalinkHost*>(i);
        if(!h->host.isEqual(srcHost))continue;
        host=h;
        break;
      }
      QString sname=QString("%1@%2").arg(uname).arg(srcHost.toString());
      if(host){
        host->setTitle(sname);
      }else{
        host=new DatalinkHost(this,sname,srcHost);
        qDebug("#%s: %s",tr("found server").toUtf8().data(),sname.toUtf8().data());
        announceTimer.setInterval(3000);
      }
      host->updateTimeout();
      continue;
    }
  }
}
//=============================================================================
void DatalinkHosts::announce(void)
{
  if(f_datalink->f_binded->value().toBool()){
    udpAnnounce->writeDatagram(QByteArray(QString("%1@server.gcs.uavos.com").arg(f_datalink->f_name->value().toString()).toUtf8()),QHostAddress::Broadcast,UDP_PORT_DISCOVER);
    announceTimer.setInterval(20000);
    //qDebug()<<"announce";
  }else{
    announceTimer.stop();
  }
}
//=============================================================================
void DatalinkHosts::serverBindedChanged()
{
  bool binded=f_datalink->f_binded->value().toBool();
  if(binded){
    announce();
    announceTimer.start();
  }else{
    announceTimer.stop();
  }
}
//=============================================================================
//=============================================================================
void DatalinkHosts::updateStats()
{
  f_alloff->setEnabled(f_list->size()>0);
}
//=============================================================================



