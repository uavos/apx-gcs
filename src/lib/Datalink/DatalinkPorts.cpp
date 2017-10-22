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
#include "DatalinkPorts.h"
#include "DatalinkPort.h"
//=============================================================================
DatalinkPorts::DatalinkPorts(Fact *parent)
  : Fact(parent,"ports",tr("Local ports"),tr("Modems and persistent remotes"),GroupItem,ConstData)
{
  _add=new DatalinkPort(this);

  _allon=new Fact(this,"allon",tr("Enable all"),tr("Turn on all communication ports"),FactItem,NoData);
  connect(_allon,&Fact::triggered,this,&DatalinkPorts::allonTriggered);
  _alloff=new Fact(this,"alloff",tr("Disable all"),tr("Turn off all communication ports"),FactItem,NoData);
  connect(_alloff,&Fact::triggered,this,&DatalinkPorts::alloffTriggered);

  load();

  connect(this,&Fact::structChanged,this,&DatalinkPorts::updateStats);
  updateStats();
}
//=============================================================================
void DatalinkPorts::allonTriggered()
{
  foreach (DatalinkPort *item, portsList) {
    item->_enabled->setValue(true);
  }
}
void DatalinkPorts::alloffTriggered()
{
  foreach (DatalinkPort *item, portsList) {
    item->_enabled->setValue(false);
  }
}
//=============================================================================
void DatalinkPorts::updateStats()
{
  bool bSz=portsList.size();
  _allon->setVisible(bSz);
  _alloff->setVisible(bSz);
  setValue(portsList.size());
}
//=============================================================================
void DatalinkPorts::addTriggered()
{
  DatalinkPort *item=new DatalinkPort(this,_add);
  portsList.append(item);
  updateStats();
  save();
  _add->defaults();
}
void DatalinkPorts::removeTriggered()
{
  DatalinkPort *item=static_cast<DatalinkPort*>(static_cast<FactTree*>(sender())->parentItem());
  portsList.removeAll(item);
  removeItem(item);
  save();
}
//=============================================================================
//=============================================================================
void DatalinkPorts::load()
{
  QSettings *settings=AppSettings::settings();
  settings->beginGroup(path());
  int size=settings->beginReadArray("port");
  for (int i = 0; i < size; ++i) {
    settings->setArrayIndex(i);
    _add->defaults();
    foreach(FactTree *i,_add->childItems()){
      Fact *fact=static_cast<Fact*>(i);
      if(fact->dataType()==ActionData)continue;
      fact->setValue(settings->value(fact->name()));
    }
    DatalinkPort *item=new DatalinkPort(this,_add);
    portsList.append(item);
  }
  _add->defaults();
  settings->endArray();
  settings->endGroup();
}
void DatalinkPorts::save()
{
  QSettings *settings=AppSettings::settings();
  settings->beginGroup(path());
  settings->remove(""); //all
  settings->beginWriteArray("port");
  int ai=0;
  foreach(const DatalinkPort *item,portsList){
    settings->setArrayIndex(ai++);
    foreach(const FactTree *i,item->childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      if((!fact->visible())||fact->dataType()==ActionData)continue;
      settings->setValue(fact->name(),fact->text());
    }
  }
  settings->endArray();
  settings->endGroup();
}
//=============================================================================

