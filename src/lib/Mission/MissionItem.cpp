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
#include "MissionItem.h"
#include "MissionField.h"
//=============================================================================
MissionItem::MissionItem(MissionGroup *parent, const QString &name, const QString &title, const QString &descr)
  : Fact(parent,name,title,descr,GroupItem,NoData),
    group(parent),
    blockUpdateCoordinate(false),
    m_course(0),
    m_time(0),
    m_distance(0),
    m_totalDistance(0),
    m_totalTime(0)
{
  f_order=new MissionField(this,"order",tr("Order"),tr("Object sequence number"),IntData);
  f_order->setValue(num()+1);
  f_order->setMin(1);
  connect(f_order,&Fact::valueChanged,this,&MissionItem::updateOrder);
  connect(this,&Fact::numChanged,this,[=](){f_order->setValue(num()+1);});


  f_latitude=new MissionField(this,"latitude",tr("Latitude"),tr("Global postition latitude"),FloatData);
  f_latitude->setUnits("lat");
  f_longitude=new MissionField(this,"longitude",tr("Longitude"),tr("Global postition longitude"),FloatData);
  f_longitude->setUnits("lon");

  f_remove=new Fact(this,"remove",tr("Remove"),"",FactItem,ActionData);
  f_remove->setValue(RemoveAction);
  connect(f_remove,&Fact::triggered,this,&Fact::remove);

  connect(f_latitude,&Fact::valueChanged,this,&MissionItem::updateCoordinate);
  connect(f_longitude,&Fact::valueChanged,this,&MissionItem::updateCoordinate);

  connect(this,&MissionItem::coordinateChanged,this,&MissionItem::updatePath);

  connect(this,&Fact::numChanged,this,&MissionItem::updatePath,Qt::QueuedConnection);

  //title
  connect(parent,&Fact::numChanged,this,&Fact::nameChanged);
  connect(this,&Fact::numChanged,this,&MissionItem::updateTitle);
  updateTitle();

  //status totals
  connect(this,&MissionItem::timeChanged,group,&MissionGroup::updateTime);
  connect(this,&MissionItem::distanceChanged,group,&MissionGroup::updateDistance);

  connect(this,&MissionItem::totalTimeChanged,this,&MissionItem::updateStatus);
  connect(this,&MissionItem::totalDistanceChanged,this,&MissionItem::updateStatus);
  updateStatus();

}
//=============================================================================
int MissionItem::missionItemType() const
{
  return group->missionItemType();
}
//=============================================================================
void MissionItem::updateTitle()
{
  setTitle(QString::number(num()+1));
}
//=============================================================================
void MissionItem::updateStatus()
{
  uint d=totalDistance();
  uint t=totalTime();
  if((d|t)==0) setStatus(QString());
  else{
    QStringList st;
    st.append(FactSystem::distanceToString(d));
    st.append(FactSystem::timeToString(t,false));
    setStatus(st.join(' '));
  }
}
//=============================================================================
void MissionItem::updateCoordinate()
{
  if(blockUpdateCoordinate)return;
  setCoordinate(QGeoCoordinate(f_latitude->value().toDouble(),f_longitude->value().toDouble()));
}
//=============================================================================
void MissionItem::updatePath()
{
  setGeoPath(getPath());
  //check to propagate updates to next items
  MissionItem *next=nextItem();
  if(next){
    QGeoCoordinate p1,p2;
    if(m_geoPath.size()>=2) p1=QGeoCoordinate(m_geoPath.path().last());
    if(next->geoPath().size()>=2) p2=QGeoCoordinate(next->geoPath().path().first());
    //p2.setAltitude(p1.altitude());
    if(next->geoPath().size()<2 || m_geoPath.size()<2 ||
       //p1!=p2
       p1.latitude()!=p2.latitude() ||
       p1.longitude()!=p2.longitude()
       ){
      next->updatePath();
    }
  }
}
//=============================================================================
void MissionItem::updateOrder()
{
  int n=f_order->value().toInt()-1;
  if(n<0)n=0;
  else if(n>=group->size())n=group->size()-1;
  if(n==num() || group->size()<2){
    f_order->setValue(num()+1);
    return;
  }
  group->moveItem(this,n,true);
}
//=============================================================================
void MissionItem::resetPath()
{
  m_geoPath=QGeoPath();
}
//=============================================================================
QGeoPath MissionItem::getPath()
{
  return QGeoPath();
}
//=============================================================================
MissionItem * MissionItem::prevItem() const
{
  return static_cast<MissionItem*>(parentItem()->child(num()-1));
}
MissionItem * MissionItem::nextItem() const
{
  return static_cast<MissionItem*>(parentItem()->child(num()+1));
}

//=============================================================================
//=============================================================================
//=============================================================================
QGeoCoordinate MissionItem::coordinate() const
{
  return m_coordinate;
}
void MissionItem::setCoordinate(const QGeoCoordinate &v)
{
  if(m_coordinate==v)return;
  m_coordinate=v;
  emit coordinateChanged();
  blockUpdateCoordinate=true;
  f_latitude->setValue(v.latitude());
  f_longitude->setValue(v.longitude());
  blockUpdateCoordinate=false;
}
QGeoPath MissionItem::geoPath() const
{
  return m_geoPath;
}
void MissionItem::setGeoPath(const QGeoPath &v)
{
  if(m_geoPath==v)return;
  m_geoPath=v;
  emit geoPathChanged();
}
double MissionItem::course() const
{
  return m_course;
}
void MissionItem::setCourse(const double &v)
{
  if(m_course==v)return;
  m_course=v;
  emit courseChanged();
}
uint MissionItem::time() const
{
  return m_time;
}
void MissionItem::setTime(uint v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
}
uint MissionItem::distance() const
{
  return m_distance;
}
void MissionItem::setDistance(uint v)
{
  if(m_distance==v)return;
  m_distance=v;
  emit distanceChanged();
}
uint MissionItem::totalDistance() const
{
  return m_totalDistance;
}
void MissionItem::setTotalDistance(uint v)
{
  if(m_totalDistance==v)return;
  m_totalDistance=v;
  emit totalDistanceChanged();
}
uint MissionItem::totalTime() const
{
  return m_totalTime;
}
void MissionItem::setTotalTime(uint v)
{
  if(m_totalTime==v)return;
  m_totalTime=v;
  emit totalTimeChanged();
}
//=============================================================================
//=============================================================================
