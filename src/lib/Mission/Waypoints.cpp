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
#include "Waypoints.h"
#include "VehicleMission.h"
#include "MissionItems.h"
//=============================================================================
Waypoints::Waypoints(VehicleMission *parent)
  : MissionItems(parent,"waypoints",tr("Waypoints"),""),
    m_distance(0),
    m_time(0)
{
}
//=============================================================================
//=============================================================================
uint Waypoints::distance() const
{
  return m_distance;
}
void Waypoints::setDistance(uint v)
{
  if(m_distance==v)return;
  m_distance=v;
  emit distanceChanged();
}
uint Waypoints::time() const
{
  return m_time;
}
void Waypoints::setTime(uint v)
{
  if(m_time==v)return;
  m_time=v;
  emit timeChanged();
}
//=============================================================================
//=============================================================================
