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
#include "AppSettingsPorts.h"
//=============================================================================
AppSettingsPort::AppSettingsPort(AppSettingsPorts *parent, const AppSettingsPort *port)
 : Fact(parent,port?"":tr("add"),port?"":tr("Add new port"),port?"":tr("Configure new port"),GroupItem,port?BoolData:NoData),
   container(parent),_new(port?false:true)
{
  if(!_new) setSection(tr("Ports"));

  connect(this,&Fact::structChanged,this,&AppSettingsPort::nameChanged);

  _enabled=new Fact(this,"enabled",tr("Enabled"),tr("Connectwhen available"),FactItem,BoolData);
  bindValue(_enabled);
  _enabled->setValue(true);

  _type=new Fact(this,"type",tr("Type"),tr("Link type"),FactItem,EnumData);
  new Fact(_type,"serial",tr("System serial port"),"",ConstItem,ItemIndexData);
  new Fact(_type,"network",tr("Network server address"),"",ConstItem,ItemIndexData);
  connect(_type,&Fact::valueChanged,this,&Fact::titleChanged);
  connect(_type,&Fact::valueChanged,this,&AppSettingsPort::typeChanged);

  _dev=new Fact(this,"path",tr("Port path"),tr("Device name"),FactItem,TextData);
  new Fact(_dev,"auto",tr("Enumerate available serial ports"),"",ConstItem,NoData);
  new Fact(_dev,"ttyUSB0","","",ConstItem,NoData);
  connect(_dev,&Fact::valueChanged,this,&Fact::titleChanged);


  _baud=new Fact(this,"baud",tr("Baud rate"),tr("Serial port speed"),FactItem,TextData);
  new Fact(_baud,"460800","","",ConstItem,NoData);
  new Fact(_baud,"230400","","",ConstItem,NoData);
  new Fact(_baud,"115200","","",ConstItem,NoData);
  connect(_baud,&Fact::valueChanged,this,&Fact::titleChanged);

  _host=new Fact(this,"host",tr("Host address"),tr("Remote server IP"),FactItem,TextData);
  connect(_host,&Fact::valueChanged,this,&Fact::titleChanged);

  if(_new){
    _save=new Fact(this,"save",tr("Save"),"",FactItem,ActionData);
    connect(_save,&Fact::triggered,parent,&AppSettingsPorts::addTriggered);
    defaults();
  }else{
    copyValuesFrom(port);
    _remove=new Fact(this,"remove",tr("Remove"),"",FactItem,ActionData);
    connect(_remove,&Fact::triggered,parent,&AppSettingsPorts::removeTriggered);
    _remove->setValue(RemoveAction);
  }

  typeChanged();

  if(!_new){
    foreach(const FactTree *i,childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      connect(fact,&Fact::valueChanged,parent,&AppSettingsPorts::save);
    }
  }

}
//=============================================================================
void AppSettingsPort::defaults()
{
  _enabled->setValue(true);
  _type->setValue(0);
  _dev->setValue("auto");
  _baud->setValue("460800");
  _host->setValue("192.168.0.123");
}
//=============================================================================
QString AppSettingsPort::name(void) const
{
  if(_new)return Fact::name();
  return QString("port%1").arg(container->ports.indexOf(const_cast<AppSettingsPort*>(this))+1);
}
QString AppSettingsPort::title(void) const
{
  if(_new)return Fact::title();
  return QString("%1: %2").arg(_type->text()).arg(_type->value().toUInt()?_host->text():_dev->text());
}
QString AppSettingsPort::descr(void) const
{
  if(_new)return Fact::descr();
  return Fact::descr();
}
//=============================================================================
void AppSettingsPort::typeChanged()
{
  bool bSerial=_type->text()=="serial";
  _dev->setVisible(bSerial);
  _baud->setVisible(bSerial);
  _host->setVisible(!bSerial);
}
//=============================================================================
//=============================================================================
AppSettingsPorts::AppSettingsPorts(Fact *parent, const QString &sect)
  : Fact(parent,"ports",tr("Ports"),tr("Modems and network servers"),GroupItem,NoData)
{
  setSection(sect);

  _add=new AppSettingsPort(this);

  _allon=new Fact(this,"allon",tr("Enable all"),tr("Turn on all communication ports"),FactItem,NoData);
  connect(_allon,&Fact::triggered,this,&AppSettingsPorts::allonTriggered);
  _alloff=new Fact(this,"alloff",tr("Disable all"),tr("Turn off all communication ports"),FactItem,NoData);
  connect(_alloff,&Fact::triggered,this,&AppSettingsPorts::alloffTriggered);

  load();

  portsChanged();
}
//=============================================================================
void AppSettingsPorts::allonTriggered()
{
  foreach (AppSettingsPort *item, ports) {
    item->_enabled->setValue(true);
  }
}
void AppSettingsPorts::alloffTriggered()
{
  foreach (AppSettingsPort *item, ports) {
    item->_enabled->setValue(false);
  }
}
//=============================================================================
void AppSettingsPorts::portsChanged()
{
  bool bPorts=ports.size();
  _allon->setVisible(bPorts);
  _alloff->setVisible(bPorts);
}
//=============================================================================
void AppSettingsPorts::addTriggered()
{
  AppSettingsPort *item=new AppSettingsPort(this,_add);
  ports.append(item);
  portsChanged();
  save();
  _add->defaults();
}
void AppSettingsPorts::removeTriggered()
{
  AppSettingsPort *item=static_cast<AppSettingsPort*>(static_cast<FactTree*>(sender())->parentItem());
  ports.removeAll(item);
  removeItem(item);
  portsChanged();
  save();
}
//=============================================================================
//=============================================================================
void AppSettingsPorts::load()
{
}
void AppSettingsPorts::save()
{
  QSettings *settings=AppSettings::settings();
  settings->beginGroup(name());
  settings->remove(""); //all
  settings->beginWriteArray("port");
  int ai=0;
  foreach(const AppSettingsPort *item,ports){
    settings->setArrayIndex(ai++);
    foreach(const FactTree *i,item->childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      if(!fact->visible())continue;
      settings->setValue(fact->name(),fact->text());
    }
  }
  settings->endArray();
  settings->endGroup();
}
//=============================================================================

