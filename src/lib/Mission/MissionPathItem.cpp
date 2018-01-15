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
#include "MissionPathItem.h"
#include "Mission.h"
#include "MissionItems.h"
#include "VehicleMission.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "Mandala.h"
//=============================================================================
MissionPathItem::MissionPathItem(MissionPathItems *parent, const QString &name, const QString &title, const QString &descr)
  : MissionOrderedItem(parent,name,title,descr),
    pathItems(parent),
    m_course(0),
    m_time(0),
    m_distance(0),
    m_totalDistance(0),
    m_totalTime(0)
{

  f_latitude=new Fact(this,"latitude",tr("Latitude"),tr("Global postition latitude"),FactItem,FloatData);
  f_latitude->setUnits("lat");
  f_longitude=new Fact(this,"longitude",tr("Longitude"),tr("Global postition longitude"),FactItem,FloatData);
  f_longitude->setUnits("lon");


  connect(f_latitude,&Fact::valueChanged,this,&MissionPathItem::updatePath);
  connect(f_longitude,&Fact::valueChanged,this,&MissionPathItem::updatePath);

  //connect(this,&Fact::nameChanged,this,&MissionPathItem::updatePath);
  connect(this,&Fact::titleChanged,this,&MissionPathItem::updatePath,Qt::QueuedConnection);

  //totals
  connect(this,&MissionPathItem::timeChanged,pathItems,&MissionPathItems::updateTime);
  connect(this,&MissionPathItem::distanceChanged,pathItems,&MissionPathItems::updateDistance);

  connect(this,&MissionPathItem::totalTimeChanged,this,&MissionPathItem::updateStatus);
  connect(this,&MissionPathItem::totalDistanceChanged,this,&MissionPathItem::updateStatus);
  updateStatus();

  //FactSystem::instance()->jsSync(this);
}
//=============================================================================
void MissionPathItem::updateStatus()
{
  QStringList st;
  st.append(FactSystem::distanceToString(totalDistance()));
  st.append(FactSystem::timeToString(totalTime(),false));
  setStatus(st.join(' '));
}
//=============================================================================
void MissionPathItem::updatePath()
{
  QGeoPath p=getPath();
  if(m_travelPath!=p){
    m_travelPath=p;
    emit travelPathChanged();
  }
  //check to propagate updates to next items
  MissionPathItem *next=nextItem<MissionPathItem*>();
  if(next && (next->travelPath().path().size()<2 || m_travelPath.size()<2 || next->travelPath().path().first()!=m_travelPath.path().last())){
    next->updatePath();
  }
}
//=============================================================================
void MissionPathItem::resetPath()
{
  m_travelPath=QGeoPath();
}
//=============================================================================
QGeoPath MissionPathItem::getPath()
{
  QGeoPath p;
  const double spd=5;
  MissionPathItem *prev=prevItem<MissionPathItem*>();
  double distance=0;
  double azimuth=0;
  if(prev){
    QGeoCoordinate p1(prev->f_latitude->value().toDouble(),prev->f_longitude->value().toDouble());
    QGeoCoordinate p2(f_latitude->value().toDouble(),f_longitude->value().toDouble());
    p.addCoordinate(p1);
    p.addCoordinate(p2);
    distance=p1.distanceTo(p2);
    azimuth=p1.azimuthTo(p2);
  }
  //update properties
  setDistance(distance);
  setTime(distance/spd);
  setCourse(azimuth);
  return p;
}
//=============================================================================
//=============================================================================
QGeoPath MissionPathItem::travelPath() const
{
  return m_travelPath;
}
double MissionPathItem::course() const
{
  return m_course;
}
void MissionPathItem::setCourse(const double &v)
{
  if(m_course==v)return;
  m_course=v;
  emit courseChanged();
}
uint MissionPathItem::time() const
{
  return m_time;
}
void MissionPathItem::setTime(uint v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
}
uint MissionPathItem::distance() const
{
  return m_distance;
}
void MissionPathItem::setDistance(uint v)
{
  if(m_distance==v)return;
  m_distance=v;
  emit distanceChanged();
}
uint MissionPathItem::totalDistance() const
{
  return m_totalDistance;
}
void MissionPathItem::setTotalDistance(uint v)
{
  if(m_totalDistance==v)return;
  m_totalDistance=v;
  emit totalDistanceChanged();
}
uint MissionPathItem::totalTime() const
{
  return m_totalTime;
}
void MissionPathItem::setTotalTime(uint v)
{
  if(m_totalTime==v)return;
  m_totalTime=v;
  emit totalTimeChanged();
}
//=============================================================================
//=============================================================================
