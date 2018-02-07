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
#include "Datalink.h"
//=============================================================================
DatalinkPorts::DatalinkPorts(Datalink *parent)
  : Fact(parent,"ports",tr("Local ports"),tr("Modems and persistent remotes"),GroupItem,ConstData),
    f_datalink(parent)
{
  setIcon("usb");
  model()->setFlat(true);

  f_add=new DatalinkPort(this);
  f_add->setIcon("plus-circle");
  connect(f_add,&Fact::triggered,f_add,&DatalinkPort::defaults);

  f_allon=new Fact(this,"allon",tr("Enable all"),tr("Turn on all communication ports"),FactItem,NoData);
  f_allon->setIcon("power-plug");
  f_alloff=new Fact(this,"alloff",tr("Disable all"),tr("Turn off all communication ports"),FactItem,NoData);
  f_alloff->setIcon("power-plug-off");

  f_list=new Fact(this,"list",tr("Ports"),tr("Configured ports"),SectionItem,ConstData);
  bind(f_list);

  load();

  connect(f_list,&Fact::sizeChanged,this,&DatalinkPorts::updateStats);
  updateStats();
}
//=============================================================================
void DatalinkPorts::updateStats()
{
  uint bSz=f_list->size();
  f_allon->setVisible(bSz);
  f_alloff->setVisible(bSz);
}
//=============================================================================
void DatalinkPorts::addTriggered()
{
  new DatalinkPort(this,f_add);
  updateStats();
  save();
  f_add->defaults();
}
void DatalinkPorts::removeTriggered()
{
  f_list->removeItem(static_cast<FactTree*>(sender())->parentItem());
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
    f_add->defaults();
    foreach(FactTree *i,f_add->childItems()){
      Fact *fact=static_cast<Fact*>(i);
      fact->setValue(settings->value(fact->name()));
    }
    new DatalinkPort(this,f_add);
  }
  f_add->defaults();
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
  foreach (FactTree *i, f_list->childItems()) {
    DatalinkPort *port=static_cast<DatalinkPort*>(i);
    settings->setArrayIndex(ai++);
    foreach(const FactTree *i,port->childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      if(!fact->visible())continue;
      settings->setValue(fact->name(),fact->text());
    }
  }
  settings->endArray();
  settings->endGroup();
}
//=============================================================================
void DatalinkPorts::forward(DatalinkPort *src, const QByteArray &ba)
{
  foreach (FactTree *i, f_list->childItems()) {
    DatalinkPort *port=static_cast<DatalinkPort*>(i);
    if(port==src) continue;
    port->sendPacket(ba);
  }
}
//=============================================================================

