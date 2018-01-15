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
#include "MissionPathItems.h"
#include "MissionPathItem.h"
#include "VehicleMission.h"
#include "MissionItems.h"
//=============================================================================
MissionPathItems::MissionPathItems(VehicleMission *parent, const QString &name, const QString &title, const QString &descr)
  : MissionItems(parent,name,title,descr),
    m_distance(0),
    m_time(0)
{
  connect(this,&MissionPathItems::distanceChanged,this,&MissionPathItems::updateDescr);
  connect(this,&MissionPathItems::timeChanged,this,&MissionPathItems::updateDescr);
  updateDescr();
}
//=============================================================================
void MissionPathItems::updateDescr()
{
  QStringList st;
  st.append(FactSystem::distanceToString(distance()));
  st.append(FactSystem::timeToString(time(),false));
  setDescr(st.join(' '));
}
//=============================================================================
void MissionPathItems::updateTime()
{
  uint v=0;
  for(int i=0;i<childItems().size();++i){
    MissionPathItem *wp=static_cast<MissionPathItem*>(child(i));
    v+=wp->time();
    wp->setTotalTime(v);
  }
  setTime(v);
}
void MissionPathItems::updateDistance()
{
  uint v=0;
  for(int i=0;i<childItems().size();++i){
    MissionPathItem *wp=static_cast<MissionPathItem*>(child(i));
    v+=wp->distance();
    wp->setTotalDistance(v);
  }
  setDistance(v);
}
//=============================================================================
uint MissionPathItems::distance() const
{
  return m_distance;
}
void MissionPathItems::setDistance(uint v)
{
  if(m_distance==v)return;
  m_distance=v;
  emit distanceChanged();
}
uint MissionPathItems::time() const
{
  return m_time;
}
void MissionPathItems::setTime(uint v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
}
//=============================================================================
//=============================================================================
