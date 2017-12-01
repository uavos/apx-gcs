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
#include "FactSystem.h"
#include "AppSettings.h"
#include "Datalink.h"
#include "DatalinkPort.h"
#include "DatalinkPorts.h"
#include "DatalinkHosts.h"
#include "DatalinkClients.h"
#include "DatalinkClient.h"
#include "DatalinkStats.h"
#include "Mandala.h"
//=============================================================================
Datalink::Datalink(FactSystem *parent)
  : Fact(parent,"datalink",tr("Datalink"),tr("Communication and networks"),GroupItem,NoData),
    m_valid(false),m_online(false),m_errcnt(0)
{
  _instance=this;

  setSection(FactSystem::ApplicationSection);

  Fact *item;
  QString sect;
  QSettings *settings=AppSettings::settings();


  f_readonly=new AppSettingFact(settings,this,"readonly",tr("Read only"),tr("Block all uplink data"),sect,BoolData,false);
  connect(f_readonly,&Fact::valueChanged,this,&Datalink::readonlyChanged);

  f_listen=new AppSettingFact(settings,this,"listen",tr("Server active"),tr("Enable network features"),sect,BoolData,true);

  f_binded=new Fact(this,"binded",tr("Server listening"),"",FactItem,NoData);
  f_binded->setEnabled(false);
  f_binded->setVisible(false);


  f_hosts=new DatalinkHosts(this);
  f_ports=new DatalinkPorts(this);
  f_clients=new DatalinkClients(this);

  connect(f_clients,&DatalinkClients::bindError,f_hosts,&DatalinkHosts::connectLocalhost);

  f_stats=new DatalinkStats(this);


  sect=tr("Server settings");
  //guess user name
  QString sname="user";
  foreach(QString s,QProcess::systemEnvironment()){
    if(!s.startsWith("USER"))continue;
    s=s.mid(s.indexOf('=')+1).trimmed();
    if(s.isEmpty())break;
    sname=s;
    break;
  }

  f_name=new AppSettingFact(settings,this,"name",tr("Name"),tr("Local server customized name"),sect,TextData,sname);
  f_pass=new AppSettingFact(settings,this,"pass",tr("Password"),tr("Local server access password"),sect,TextData);
  f_extctr=new AppSettingFact(settings,this,"extctr",tr("Allow external controls"),tr("Don't block uplink from remote clients"),sect,BoolData,true);
  f_hbeat=new AppSettingFact(settings,this,"hbeat",tr("Send heartbeat"),tr("Vehicle datalink available status"),sect,BoolData,true);

  sect=tr("Internet");
  item=new AppSettingFact(settings,this,"proxy",tr("HTTP proxy"),tr("Proxy for web data requests"),sect,TextData);


  AppSettingFact::loadSettings(this);


  //heartbeat timer
  connect(f_hbeat,&Fact::valueChanged,this,&Datalink::hbeatChanged);
  connect(&heartbeatTimer,SIGNAL(timeout()),this,SLOT(heartbeatTimeout()));
  heartbeatTimer.setInterval(1500);
  hbeatChanged();

  //online timeout
  onlineTimer.setSingleShot(true);
  onlineTimer.setInterval(7000);
  connect(&onlineTimer,&QTimer::timeout,this,[=](){ setOnline(false); });
  connect(this,&Datalink::read,this,[=](){ setOnline(true); });
  connect(this,&Datalink::onlineChanged,this,[=](){
    if(!m_online)setErrcnt(0);
    FactSystem::instance()->sound(m_online?"connected":"error");
  });

  connect(this,&Datalink::read,this,[=](){ setValid(true); });

  parent->jsSync(this);
}
Datalink * Datalink::_instance=NULL;
//=============================================================================
void Datalink::readonlyChanged()
{
  bReadOnly=static_cast<Fact*>(sender())->value().toBool();
  if(bReadOnly)qDebug("%s",tr("Read only datalink").toUtf8().data());
  else qDebug("%s",tr("Uplink allowed").toUtf8().data());
}
//=============================================================================
//=============================================================================
void Datalink::heartbeatTimeout()
{
  if(!f_hbeat->value().toBool())return;
  //f_upcnt->setValue(f_upcnt->value().toUInt()+1);
  QByteArray ba;
  ba.append((char)idx_ping).append((char)0);
  emit transmittedData(ba);
  emit sendPacketToPorts(ba);
}
void Datalink::hbeatChanged()
{
  if(f_hbeat->value().toBool())heartbeatTimer.start();
  else heartbeatTimer.stop();
}
//=============================================================================
//=============================================================================
void Datalink::packetReceivedFromClient(const QByteArray &ba)
{
  //uplink read from a client
  emit receivedData(ba);

  if(!f_extctr->value().toBool())return;
  emit read(ba);
  emit sendPacketToPorts(ba);   //share to all local ports
  emit sendPacketToHosts(ba);   //share to remote gcs
  f_clients->forward(static_cast<DatalinkClient*>(sender()),ba);   //share to other clients
}
void Datalink::packetReceivedFromHost(const QByteArray &ba)
{
  //downlink/uplink from remote server
  emit receivedData(ba);

  emit read(ba);
  emit sendPacketToClients(ba); //share downlink from remote host
}
void Datalink::packetReceivedFromPort(const QByteArray &ba)
{
  //downlink from local port
  emit receivedData(ba);

  emit read(ba);
  DatalinkPort *port=static_cast<DatalinkPort*>(sender());
  if(port->f_local->value().toBool()==false){
    emit sendPacketToClients(ba); //share all downlink from port
    emit sendPacketToHosts(ba);   //share all downlink from port
  }
}
//=============================================================================
//=============================================================================
void Datalink::write(const QByteArray &ba)
{
  //qDebug()<<"W"<<ba.toHex().toUpper();
  //if(bReadOnly)return;
  emit transmittedData(ba);

  //send uplink from gcs plugins
  emit sendPacketToPorts(ba);
  emit sendPacketToHosts(ba);   //send to remote gcs
  emit sendPacketToClients(ba); //sync uplink
}
//=============================================================================
//=============================================================================
bool Datalink::valid() const
{
  return m_valid;
}
void Datalink::setValid(const bool &v)
{
  if(m_valid==v)return;
  m_valid=v;
  emit validChanged();
}
bool Datalink::online() const
{
  return m_online;
}
void Datalink::setOnline(const bool &v)
{
  if(v) onlineTimer.start();
  if(m_online==v)return;
  m_online=v;
  emit onlineChanged();
}
uint Datalink::errcnt() const
{
  return m_errcnt;
}
void Datalink::setErrcnt(const uint &v)
{
  if(m_errcnt==v)return;
  m_errcnt=v;
  emit errcntChanged();
}
//=============================================================================

