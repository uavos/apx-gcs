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
#include "VehicleWarnings.h"
#include "Vehicle.h"
//=============================================================================
VehicleWarnings::VehicleWarnings(Vehicle *parent)
  : Fact(parent,"warnings",tr("Warnings"),tr("Malfunctions and warnings list"),GroupItem,ConstData),
    showNum(0)
{
  setIcon("alert");

  model()->setFlat(true);

  f_clear=new FactAction(this,"clear",tr("Clear"),tr("Remove all messages from list"),"notification-clear-all");
  f_clear->setEnabled(false);
  connect(f_clear,&FactAction::triggered,this,&Fact::removeAll);

  connect(this,&Fact::sizeChanged,this,[=](){
    f_clear->setEnabled(size()>0);
  });

  showTimer.setSingleShot(true);
  showTimer.setInterval(5000);
  connect(&showTimer,&QTimer::timeout,this,&VehicleWarnings::showTimerTimeout);
}
//=============================================================================
void VehicleWarnings::warning(const QString &msg)
{
  createItem(msg,WARNING);
  FactSystem::instance()->sound("warning");
}
void VehicleWarnings::error(const QString &msg)
{
  createItem(msg,ERROR);
  FactSystem::instance()->sound("error");
}
//=============================================================================
Fact * VehicleWarnings::createItem(const QString &msg, MsgType kind)
{
  Fact *fact=NULL;
  if(size()>0){
    fact=static_cast<Fact*>(childItems().first());
    if(fact->title()!=msg || fact->userData.toInt()!=kind) fact=NULL;
  }
  if(!fact){
    fact=new Fact(this,"item#",msg,"",FactItem,ConstData);
    removeItem(fact,false);
    insertItem(0,fact);
    if(size()>100)removeItem(childItems().last());
    fact->setValue(1);
    fact->userData=kind;
    switch(kind){
      case INFO: fact->setStatus(tr("Information")); break;
      case WARNING: fact->setStatus(tr("Warning")); break;
      case ERROR: fact->setStatus(tr("Error")); break;
    }
    fact->setDescr(QDateTime::currentDateTime().toString());
    connect(fact,&Fact::destroyed,this,[=](){
      showMap.remove(fact);
      showList.removeAll(fact);
    });
  }else{
    fact->setValue(fact->value().toUInt()+1);
  }
  foreach (Fact *f, showList) {
    if(f->title()!=fact->title())continue;
    showList.removeAll(f);
    showMap.remove(f);
    break;
  }
  emit show(fact->title(),kind);
  showList.insert(showNum,fact);
  showMap.insert(fact,0);
  showNum=showList.indexOf(fact);
  showTimer.stop();
  showTimerTimeout();
  return fact;
}
//=============================================================================
void VehicleWarnings::showTimerTimeout()
{
  if(showList.isEmpty())return;
  //continuously show all facts
  if(showNum>=showList.size())showNum=0;
  Fact * fact=showList.at(showNum);
  emit showMore(fact->title(),(MsgType)fact->userData.toInt());
  //qDebug()<<fact->title();

  //next item
  if(++showMap[fact]>=3) {
    showList.removeAll(fact);
    showMap.remove(fact);
  }else showNum++;
  if(showNum>=showList.size())showNum=0;
  if(!showList.isEmpty()) showTimer.start();
}
//=============================================================================
//=============================================================================
