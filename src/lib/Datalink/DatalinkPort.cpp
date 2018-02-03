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
#include "DatalinkPort.h"
#include "DatalinkPorts.h"
#include "DatalinkHost.h"
#include "Datalink.h"
#include "Serial.h"
//=============================================================================
DatalinkPort::DatalinkPort(DatalinkPorts *parent, const DatalinkPort *port)
 : Fact(port?parent->f_list:parent,port?"port#":tr("add"),port?"":tr("Add new port"),port?"":tr("Configure new port"),GroupItem,port?BoolData:NoData),
   f_ports(parent),
   if_host(NULL),if_serial(NULL),
   _new(port?false:true)
{
  f_enabled=new Fact(this,"enabled",tr("Enabled"),tr("Connect when available"),FactItem,BoolData);
  if(!_new)bind(f_enabled);

  f_type=new Fact(this,"type",tr("Type"),tr("Link type"),FactItem,EnumData);
  f_type->setEnumStrings(QStringList()
                         <<"serial"
                         <<"usb"
                         <<"network" );
  f_type->setEnabled(_new);

  f_dev=new Fact(this,"path",tr("Port path"),tr("Device name"),FactItem,TextData);
  f_dev->setEnumStrings(QStringList() <<"auto" );
  f_dev->setEnabled(_new);


  f_baud=new Fact(this,"baud",tr("Baud rate"),tr("Serial port speed"),FactItem,TextData);
  f_baud->setEnumStrings(QStringList()
                         <<"460800"
                         <<"230400"
                         <<"115200" );

  f_host=new Fact(this,"host",tr("Host address"),tr("Remote server IP"),FactItem,TextData);
  connect(static_cast<Datalink*>(parent->parentItem())->f_hosts->f_list,&Fact::sizeChanged,this,&DatalinkPort::syncHostEnum,Qt::QueuedConnection);

  f_local=new Fact(this,"local",tr("Local data only"),tr("Never share received data with other connections"),FactItem,BoolData);

  if(_new){
    f_save=new FactAction(this,"save",tr("Save"),"",FactAction::ApplyAction);
    connect(f_save,&FactAction::triggered,parent,&DatalinkPorts::addTriggered);
    defaults();
  }else{
    setSection(parent->f_list->title());
    copyValuesFrom(port);
    f_remove=new FactAction(this,"remove",tr("Remove"),"",FactAction::RemoveAction);
    connect(f_remove,&FactAction::triggered,parent,&DatalinkPorts::removeTriggered);
    connect(this,&Fact::childValueChanged,parent,&DatalinkPorts::save);
    connect(parent,&Fact::sizeChanged,this,&DatalinkPort::updateStats);
    connect(parent->f_allon,&Fact::triggered,this,&DatalinkPort::enable);
    connect(parent->f_alloff,&Fact::triggered,this,&DatalinkPort::disable);

    connect(f_enabled,&Fact::valueChanged,this,&DatalinkPort::enabledChanged);

    connect(this,&DatalinkPort::packetReceived,parent->f_datalink,&Datalink::packetReceivedFromPort);
    connect(parent->f_datalink,&Datalink::sendPacketToPorts,this,&DatalinkPort::sendPacket);

    QTimer::singleShot(100,this,SLOT(enabledChanged()));
  }

  connect(this,&Fact::childValueChanged,this,&DatalinkPort::updateStats);
  updateStats();
}
//=============================================================================
void DatalinkPort::defaults()
{
  f_enabled->setValue(true);
  f_type->setValue(0);
  f_dev->setValue("auto");
  f_baud->setValue("460800");
  f_host->setValue("192.168.0.123");
  f_local->setValue(false);
  syncDevEnum();
}
//=============================================================================
void DatalinkPort::updateStats()
{
  bool bSerial=f_type->text()=="serial";
  bool bUsb=f_type->text()=="usb";
  f_dev->setVisible(bSerial);
  f_baud->setVisible(bSerial);
  f_host->setVisible((!bSerial)&&(!bUsb));
  if(!_new){
    setTitle(QString("%1: %2").arg(f_type->text()).arg(f_type->value().toUInt()==0?f_dev->text():f_type->value().toUInt()==1?"auto":f_host->text()));
    setDescr(f_local->value().toBool()?tr("Local data"):tr("Share received data"));
  }
}
//=============================================================================
void DatalinkPort::syncDevEnum()
{
  QStringList st=f_dev->enumStrings();
  bool upd=false;
  foreach(QSerialPortInfo spi,QSerialPortInfo::availablePorts()){
    if(st.contains(spi.portName()))continue;
    st.append(spi.portName());
    upd=true;
  }
  if(!upd)return;
  f_dev->setEnumStrings(st);
}
void DatalinkPort::syncHostEnum()
{
  QStringList st=f_host->enumStrings();
  bool upd=false;
  foreach(const FactTree *i, static_cast<const Fact*>(sender())->childItems()){
    const Fact *f=static_cast<const Fact*>(i);
    if(st.contains(f->text()))continue;
    st.append(f->text());
    upd=true;
  }
  if(!upd)return;
  f_host->setEnumStrings(st);
}
//=============================================================================
void DatalinkPort::enable()
{
  f_enabled->setValue(true);
}
void DatalinkPort::disable()
{
  f_enabled->setValue(false);
}
//=============================================================================
void DatalinkPort::sendPacket(const QByteArray &ba)
{
  if(if_host){
    if_host->sendPacket(ba);
    return;
  }
  if(if_serial){
    if_serial->send(ba);
    return;
  }
}
void DatalinkPort::ifacePacketReceived(const QByteArray &ba)
{
  emit packetReceived(ba);
}
//=============================================================================
//=============================================================================
void DatalinkPort::enabledChanged()
{
  if(f_enabled->value().toBool()){
    connectPort();
  }else{
    disconnectAll();
  }
}
void DatalinkPort::connectPort()
{
  if(active()){
    disconnectAll();
  }

  if(f_type->text()=="serial"){
    if(!if_serial){
      if_serial=new Serial(f_dev->text(),f_baud->value().toUInt(),this,true);
      connect(if_serial,&Serial::received,this,&DatalinkPort::ifacePacketReceived);
      connect(if_serial,&Serial::connected,this,&DatalinkPort::serialConnected);
      connect(if_serial,&Serial::disconnected,this,&DatalinkPort::serialDisconnected);
      setStatus(tr("Connecting"));
    }
  }else if(f_type->text()=="network"){
    if(!if_host) {
      if_host=f_ports->f_datalink->f_hosts->registerHost(QHostAddress(f_host->value().toString()),name()+": "+f_host->text(),true);
      connect(if_host,&DatalinkSocket::packetReceived,this,&DatalinkPort::ifacePacketReceived);
      connect(if_host,&DatalinkSocket::statusChanged,this,&DatalinkPort::hostStatusChanged);
    }
  }
  if(if_host){
    if_host->connectToServer();
    return;
  }
}
//=============================================================================
void DatalinkPort::disconnectAll()
{
  if(if_host){
    if_host->disconnectAll();
    if_host->remove();
    if_host=NULL;
  }
  if(if_serial){
    qDebug("%s: %s",tr("Serial port closed").toUtf8().data(),status().toUtf8().data());
    if_serial->deleteLater();
    if_serial=NULL;
  }
  setStatus(QString());
}
//=============================================================================
bool DatalinkPort::active() const
{
  return (if_host && if_host->active()) || if_serial;
}
//=============================================================================
//=============================================================================
void DatalinkPort::serialConnected(QString pname)
{
  setStatus(pname);
}
void DatalinkPort::serialDisconnected()
{
  setStatus(tr("Disconnected"));
}
void DatalinkPort::hostStatusChanged()
{
  if(if_host)setStatus(if_host->status());
  else setStatus(QString());
}
//=============================================================================
