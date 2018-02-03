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
#include "AppSettings.h"
#include "tcp_ports.h"
//=============================================================================
DatalinkHosts::DatalinkHosts(Datalink *parent)
  : Fact(parent,"hosts",tr("Remote servers"),tr("Discovered remote hosts"),GroupItem,NoData),
    m_connectedCount(0)
{
  setIconSource("download-network");
  model()->setFlat(true);

  f_datalink=parent;
  f_localhost=NULL;

  QSettings *settings=AppSettings::settings();

  f_add=new Fact(this,"add",tr("Connect to host"),tr("Create new connection"),GroupItem,NoData);
  f_add->setIconSource("plus-network");
  f_host=new AppSettingFact(settings,f_add,"host",tr("Host address"),tr("IP address of remote server"),"",TextData,QString());
  //f_host=new Fact(f_add,"host",tr("Host address"),tr("IP address of remote server"),FactItem,TextData);
  f_connect=new FactAction(f_add,"connect",tr("Connect"),"",FactAction::ApplyAction);
  connect(f_connect,&FactAction::triggered,this,&DatalinkHosts::connectTriggered);

  AppSettingFact::loadSettings(this);

  f_alloff=new FactAction(this,"alloff",tr("Disconnect all"),tr("Close all remote server connections"),FactAction::NormalAction,"lan-disconnect");

  f_list=new Fact(this,"list",tr("Servers list"),tr("Found servers"),SectionItem,ConstData);
  //bind(f_list);
  connect(f_list,&Fact::sizeChanged,this,&DatalinkHosts::updateStats);
  connect(f_list,&Fact::sizeChanged,this,&DatalinkHosts::availableCountChanged);
  connect(this,&DatalinkHosts::availableCountChanged,this,&DatalinkHosts::updateConnectedStatus);
  connect(this,&DatalinkHosts::connectedCountChanged,this,&DatalinkHosts::updateConnectedStatus);

  //udp discover
  udpReader=new QUdpSocket(this);
  connect(udpReader,&QUdpSocket::readyRead,this, &DatalinkHosts::udpRead);
  tryBind();

  udpAnnounce=new QUdpSocket(this);
  announceTimer.setInterval(20000);
  connect(&announceTimer,&QTimer::timeout,this,&DatalinkHosts::announce);
  connect(f_datalink->f_binded,&Fact::valueChanged,this,&DatalinkHosts::serverBindedChanged);

  // QML types register
  //qmlRegisterUncreatableType<Fact>("GCS.DatalinkHosts", 1, 0, "DatalinkHosts", "Reference only");

  updateStats();
}
//=============================================================================
DatalinkHost * DatalinkHosts::registerHost(QHostAddress addr, QString sname, bool bPort)
{
  DatalinkHost *host=hostByAddr(addr);
  if(host){
    if(host->enabled()) host->setTitle(sname);
    if(bPort){ //found host which is forced to be port
      disconnect(host,&DatalinkSocket::packetReceived,f_datalink,&Datalink::packetReceivedFromHost);
      disconnect(f_datalink,&Datalink::sendPacketToHosts,host,&DatalinkSocket::sendPacket);
      disconnect(f_alloff,&FactAction::triggered,host,&DatalinkHost::disconnectAll);
    }
  }else{
    host=new DatalinkHost(this,sname,addr);
    if(!bPort){
      qDebug("#%s: %s",tr("found server").toUtf8().data(),sname.toUtf8().data());
      announceTimer.setInterval(3000);
      connect(host,&DatalinkSocket::packetReceived,f_datalink,&Datalink::packetReceivedFromHost);
      connect(f_datalink,&Datalink::sendPacketToHosts,host,&DatalinkSocket::sendPacket);
      connect(f_alloff,&FactAction::triggered,host,&DatalinkHost::disconnectAll);
    }
  }
  if(bPort){
    host->setTitle(sname);
    host->setEnabled(false);
  }
  return host;
}
DatalinkHost * DatalinkHosts::hostByAddr(QHostAddress addr)
{
  foreach (FactTree *i , f_list->childItems()) {
    DatalinkHost *h=static_cast<DatalinkHost*>(i);
    if(!h->host.isEqual(addr))continue;
    return h;
  }
  return NULL;
}
//=============================================================================
void DatalinkHosts::connectLocalhost()
{
  if(f_localhost)return;
  //create default localhost for second app on one machine
  DatalinkHost *host=hostByAddr(QHostAddress::LocalHost);
  if(!host){
    host=new DatalinkHost(this,"localhost",QHostAddress::LocalHost);
    qDebug("%s",tr("Connecting to localhost by default").toUtf8().data());
  }
  if(host->enabled()){
    f_localhost=host;
    connect(host,&DatalinkSocket::packetReceived,f_datalink,&Datalink::packetReceivedFromHost);
    connect(f_datalink,&Datalink::sendPacketToHosts,host,&DatalinkSocket::sendPacket);
    connect(f_alloff,&FactAction::triggered,host,&DatalinkHost::disconnectAll);
  }
  if(!host->connectionActive())host->connectToServer();
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
      if(f_datalink->f_binded->value().toBool() && DatalinkSocket::isLocalHost(srcHost))continue;
      const QString &uname=QString(datagram).left(datagram.indexOf('@'));
      //try to find and update existing
      QString sname=QString("%1@%2").arg(uname).arg(srcHost.toString());
      DatalinkHost *host=registerHost(srcHost,sname);
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
  //count connected
  int cnt=0;
  foreach (FactTree *i , f_list->childItems()) {
    DatalinkHost *h=static_cast<DatalinkHost*>(i);
    if(h->connectionActive())cnt++;
  }
  f_alloff->setEnabled(cnt>0);
  if(m_connectedCount!=cnt){
    m_connectedCount=cnt;
    emit connectedCountChanged();
  }
}
//=============================================================================
void DatalinkHosts::updateConnectedStatus()
{
  //count connected
  int cnt=0;
  foreach (FactTree *i , f_list->childItems()) {
    DatalinkHost *h=static_cast<DatalinkHost*>(i);
    if(h->connectionActive())cnt++;
  }
  f_alloff->setEnabled(cnt>0);
  if(m_connectedCount!=cnt){
    m_connectedCount=cnt;
    emit connectedCountChanged();
  }
  cnt=availableCount();
  if(cnt>0) setStatus(QString("%1/%2").arg(m_connectedCount).arg(availableCount()));
  else setStatus(QString());
}
//=============================================================================
void DatalinkHosts::connectTriggered()
{
  connectToServer(QHostAddress(f_host->text()));
}
//=============================================================================
bool DatalinkHosts::connectToServer(QHostAddress haddr)
{
  if(haddr.isNull())return false;
  //if(addr.isNull() || addr.isLoopback())return;
  DatalinkHost *host=NULL;
  foreach (FactTree *i , f_list->childItems()) {
    DatalinkHost *h=static_cast<DatalinkHost*>(i);
    if(!h->host.isEqual(haddr))continue;
    host=h;
    break;
  }
  if(!host) host=new DatalinkHost(this,haddr.toString(),haddr);
  host->connectToServer();
  return true;
}
//=============================================================================
//=============================================================================
int DatalinkHosts::connectedCount() const
{
  return m_connectedCount;
}
int DatalinkHosts::availableCount() const
{
  return f_list->size();
}
//=============================================================================



