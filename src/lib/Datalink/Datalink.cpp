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
#include "AppSettings.h"
#include "Datalink.h"
#include "DatalinkPort.h"
#include "DatalinkPorts.h"
#include "DatalinkHosts.h"
#include "DatalinkClients.h"
#include "Mandala.h"
//=============================================================================
Datalink::Datalink(FactSystem *parent)
  : Fact(parent->tree(),"datalink",tr("Datalink"),tr("Communication and networks"),RootItem,NoData)
{
  setSection(FactSystem::ApplicationSection);

  Fact *item;
  QString sect;
  QSettings *settings=AppSettings::settings();


  f_readonly=new AppSettingFact(settings,this,"readonly",tr("Read only"),tr("Block all uplink data"),sect,BoolData,false);
  connect(f_readonly,&Fact::valueChanged,this,&Datalink::readonlyChanged);

  f_active=new Fact(this,"active",tr("Server active"),"",FactItem,BoolData);
  //f_active->setEnabled(false);
  f_binded=new Fact(this,"binded",tr("Server listening"),"",FactItem,ConstData);
  //f_binded->setEnabled(false);


  f_hosts=new DatalinkHosts(this);
  f_ports=new DatalinkPorts(this);
  f_clients=new DatalinkClients(this);

  f_stats=new Fact(this,"stats",tr("Statistics"),"",GroupItem,NoData);
  sect=tr("Downlink");
  f_dncnt=new Fact(f_stats,"dncnt",tr("Downlink packets"),"",FactItem,ConstData);
  f_dnrate=new Fact(f_stats,"dnrate",tr("Downlink rate"),"",FactItem,ConstData);
  sect=tr("Uplink");
  f_upcnt=new Fact(f_stats,"upcnt",tr("Uplink packets"),"",FactItem,ConstData);
  f_uprate=new Fact(f_stats,"upcnt",tr("Uplink rate"),"",FactItem,ConstData);


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

}
//=============================================================================
void Datalink::readonlyChanged()
{
  if(static_cast<Fact*>(sender())->value().toBool())qDebug("%s",tr("Read only datalink").toUtf8().data());
  else qDebug("%s",tr("Uplink allowed").toUtf8().data());
}
//=============================================================================
//=============================================================================
void Datalink::heartbeatTimeout()
{
  if(!f_active->value().toBool())return;
  if(!f_hbeat->value().toBool())return;
  f_upcnt->setValue(f_upcnt->value().toUInt()+1);
  emit sendPacketToPorts(QByteArray().append((char)idx_ping).append((char)0));
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
}
void Datalink::packetReceivedFromHost(const QByteArray &ba)
{
  f_dncnt->setValue(f_dncnt->value().toUInt()+1);
}
void Datalink::packetReceivedFromPort(const QByteArray &ba)
{
  DatalinkPort *port=static_cast<DatalinkPort*>(sender());
  f_dncnt->setValue(f_dncnt->value().toUInt()+1);
}
//=============================================================================
//=============================================================================
void Datalink::write(const QByteArray &ba)
{
  f_upcnt->setValue(f_upcnt->value().toUInt()+1);
  //send uplink from gcs plugins
  emit sendPacketToPorts(ba);
  emit sendPacketToHosts(ba);   //send to remote gcs
  emit sendPacketToClients(ba); //sync uplink
}
//=============================================================================
