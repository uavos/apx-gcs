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
#include "Datalink.h"
//=============================================================================
DatalinkPort::DatalinkPort(DatalinkPorts *parent, const DatalinkPort *port)
 : Fact(port?parent->f_list:parent,port?"port#":tr("add"),port?"":tr("Add new port"),port?"":tr("Configure new port"),GroupItem,port?BoolData:NoData),
   _new(port?false:true)
{
  _enabled=new Fact(this,"enabled",tr("Enabled"),tr("Connect when available"),FactItem,BoolData);
  if(!_new)bindValue(_enabled);

  _type=new Fact(this,"type",tr("Type"),tr("Link type"),FactItem,EnumData);
  new Fact(_type,"serial",tr("System serial port"),"",ConstItem,ItemIndexData);
  new Fact(_type,"usb",tr("Supported USB device"),"",ConstItem,ItemIndexData);
  new Fact(_type,"network",tr("Network server address"),"",ConstItem,ItemIndexData);

  _dev=new Fact(this,"path",tr("Port path"),tr("Device name"),FactItem,TextData);
  new Fact(_dev,"auto",tr("Enumerate available serial ports"),"",ConstItem,NoData);
  new Fact(_dev,"ttyUSB0","","",ConstItem,NoData);


  _baud=new Fact(this,"baud",tr("Baud rate"),tr("Serial port speed"),FactItem,TextData);
  new Fact(_baud,"460800","","",ConstItem,NoData);
  new Fact(_baud,"230400","","",ConstItem,NoData);
  new Fact(_baud,"115200","","",ConstItem,NoData);

  _host=new Fact(this,"host",tr("Host address"),tr("Remote server IP"),FactItem,TextData);
  _host->bindChilds(static_cast<Datalink*>(parent->parentItem())->f_hosts->f_list);

  _share=new Fact(this,"fwd",tr("Share data"),tr("Send all received data to other ports"),FactItem,BoolData);

  if(_new){
    _save=new Fact(this,"save",tr("Save"),"",FactItem,ActionData);
    connect(_save,&Fact::triggered,parent,&DatalinkPorts::addTriggered);
    defaults();
  }else{
    setSection(parent->f_list->title());
    copyValuesFrom(port);
    _remove=new Fact(this,"remove",tr("Remove"),"",FactItem,ActionData);
    connect(_remove,&Fact::triggered,parent,&DatalinkPorts::removeTriggered);
    _remove->setValue(RemoveAction);
    connect(this,&Fact::childValueChanged,parent,&DatalinkPorts::save);
    connect(parent,&Fact::structChanged,this,&DatalinkPort::updateStats);
    connect(parent->f_allon,&Fact::triggered,this,&DatalinkPort::enable);
    connect(parent->f_alloff,&Fact::triggered,this,&DatalinkPort::disable);
  }

  connect(this,&Fact::childValueChanged,this,&DatalinkPort::updateStats);
  updateStats();
}
//=============================================================================
void DatalinkPort::defaults()
{
  _enabled->setValue(true);
  _type->setValue(0);
  _dev->setValue("auto");
  _baud->setValue("460800");
  _host->setValue("192.168.0.123");
  _share->setValue(false);
}
//=============================================================================
void DatalinkPort::updateStats()
{
  bool bSerial=_type->text()=="serial";
  bool bUsb=_type->text()=="usb";
  _dev->setVisible(bSerial);
  _baud->setVisible(bSerial);
  _host->setVisible((!bSerial)&&(!bUsb));
  if(!_new){
    setTitle(QString("%1: %2").arg(_type->text()).arg(_type->value().toUInt()==0?_dev->text():_type->value().toUInt()==1?"auto":_host->text()));
    setDescr(_share->value().toBool()?tr("Share received data"):tr("Local data"));
  }
}
//=============================================================================
void DatalinkPort::enable()
{
  _enabled->setValue(true);
}
void DatalinkPort::disable()
{
  _enabled->setValue(false);
}
//=============================================================================
