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
#include "MissionGroup.h"
#include "VehicleMission.h"
#include "MissionItem.h"
//=============================================================================
MissionGroup::MissionGroup(VehicleMission *parent, const QString &name, const QString &title, const QString &descr)
  : Fact(parent,name,title,descr,GroupItem,NoData),
    mission(parent),
    _descr(descr),
    m_distance(0),
    m_time(0)
{
  //setSection(tr("Mission elements"));
  mission->groups.append(this);

  f_clear=new FactAction(this,"clear",tr("Clear"),tr("Remove all objects"),FactAction::RemoveAction);
  f_clear->setEnabled(false);
  connect(f_clear,&FactAction::triggered,this,&MissionGroup::clearGroup);




  //status
  connect(this,&Fact::sizeChanged,this,&MissionGroup::updateStatus);

  //time & distance
  updateTimeTimer.setSingleShot(true);
  updateTimeTimer.setInterval(1000);
  connect(&updateTimeTimer,&QTimer::timeout,this,&MissionGroup::updateTimeDo);
  updateDistanceTimer.setSingleShot(true);
  updateDistanceTimer.setInterval(1000);
  connect(&updateDistanceTimer,&QTimer::timeout,this,&MissionGroup::updateDistanceDo);

  connect(this,&Fact::sizeChanged,this,&MissionGroup::updateTime);
  connect(this,&Fact::sizeChanged,this,&MissionGroup::updateDistance);

  //descr
  connect(this,&MissionGroup::distanceChanged,this,&MissionGroup::updateDescr);
  connect(this,&MissionGroup::timeChanged,this,&MissionGroup::updateDescr);


  updateStatus();
  updateDescr();
}
//=============================================================================
void MissionGroup::updateStatus()
{
  int sz=size();
  if(sz>0)setStatus(QString("[%1]").arg(sz));
  else setStatus(QString());
  f_clear->setEnabled(sz>0);
}
//=============================================================================
void MissionGroup::updateDescr()
{
  uint d=distance();
  uint t=time();
  if((d|t)==0 && (!_descr.isEmpty())) setDescr(_descr);
  else{
    QStringList st;
    st.append(FactSystem::distanceToString(d));
    st.append(FactSystem::timeToString(t,false));
    setDescr(st.join(' '));
  }
}
//=============================================================================
void MissionGroup::updateTime()
{
  updateTimeTimer.start();
}
void MissionGroup::updateTimeDo()
{
  uint v=0;
  for(int i=0;i<childItems().size();++i){
    MissionItem *wp=static_cast<MissionItem*>(child(i));
    v+=wp->time();
    wp->setTotalTime(v);
  }
  setTime(v);
}
void MissionGroup::updateDistance()
{
  updateDistanceTimer.start();
}
void MissionGroup::updateDistanceDo()
{
  uint v=0;
  for(int i=0;i<childItems().size();++i){
    MissionItem *wp=static_cast<MissionItem*>(child(i));
    v+=wp->distance();
    wp->setTotalDistance(v);
  }
  setDistance(v);
}
//=============================================================================
uint MissionGroup::distance() const
{
  return m_distance;
}
void MissionGroup::setDistance(uint v)
{
  if(m_distance==v)return;
  m_distance=v;
  emit distanceChanged();
}
uint MissionGroup::time() const
{
  return m_time;
}
void MissionGroup::setTime(uint v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
}
//=============================================================================
void MissionGroup::add()
{
  const QGeoCoordinate &p=mission->mapCoordinate();
  if(!p.isValid()){
    qWarning("%s",tr("Click on map first").toUtf8().data());
    return;
  }
  addObject(p);
}
//=============================================================================
void MissionGroup::objectAdded(Fact *fact)
{
  QTimer::singleShot(200,fact,&Fact::trigger);
}
//=============================================================================
//=============================================================================
void MissionGroup::clearGroup()
{
  removeAll();
  setModified(true,true);
}
//=============================================================================
