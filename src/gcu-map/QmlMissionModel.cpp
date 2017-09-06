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
#include "QmlMissionModel.h"
#include "QMandala.h"
#include "Mission.h"
//=============================================================================
QmlMissionModel::QmlMissionModel(QQuickItem *parent)
 : MissionModel(parent)//,m_waypoints(this,m_waypoints_src)
{
  connect(MissionModel::waypoints,SIGNAL(addedRemoved()),this,SIGNAL(waypointsChanged()));//,Qt::QueuedConnection);
  connect(MissionModel::runways,SIGNAL(addedRemoved()),this,SIGNAL(runwaysChanged()));//,Qt::QueuedConnection);
  connect(MissionModel::taxiways,SIGNAL(addedRemoved()),this,SIGNAL(taxiwaysChanged()));//,Qt::QueuedConnection);
  connect(MissionModel::points,SIGNAL(addedRemoved()),this,SIGNAL(pointsChanged()));//,Qt::QueuedConnection);

  connect(this,SIGNAL(addedRemoved()),this,SIGNAL(waypointsChanged()));//,Qt::QueuedConnection);
  connect(this,SIGNAL(addedRemoved()),this,SIGNAL(runwaysChanged()));//,Qt::QueuedConnection);
  connect(this,SIGNAL(addedRemoved()),this,SIGNAL(taxiwaysChanged()));//,Qt::QueuedConnection);
  connect(this,SIGNAL(addedRemoved()),this,SIGNAL(pointsChanged()));//,Qt::QueuedConnection);
}
//=============================================================================
QQmlListProperty<MissionItemWp> QmlMissionModel::waypoints()
{
  return MissionModel::waypoints->objectsList();
}
QQmlListProperty<MissionItemRw> QmlMissionModel::runways()
{
  return MissionModel::runways->objectsList();
}
QQmlListProperty<MissionItemTw> QmlMissionModel::taxiways()
{
  return MissionModel::taxiways->objectsList();
}
QQmlListProperty<MissionItemPi> QmlMissionModel::points()
{
  return MissionModel::points->objectsList();
}
//=============================================================================
