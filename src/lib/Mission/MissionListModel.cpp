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
#include "VehicleMission.h"
#include "MissionListModel.h"
#include "Waypoints.h"
#include "Waypoint.h"
#include "Runways.h"
#include "Runway.h"
#include "Taxiways.h"
#include "Taxiway.h"
#include "Points.h"
#include "Point.h"
//=============================================================================
MissionListModel::MissionListModel(VehicleMission *parent)
 : QAbstractListModel(parent),
   mission(parent)
{
  groups << mission->f_runways;
  groups << mission->f_waypoints;
  groups << mission->f_points;
  groups << mission->f_taxiways;

  foreach (Fact *fact, groups) {
    connect(fact,&Fact::itemToBeInserted,this,&MissionListModel::itemToBeInserted);
    connect(fact,&Fact::itemInserted,this,&MissionListModel::itemInserted);

    connect(fact,&Fact::itemToBeRemoved,this,&MissionListModel::itemToBeRemoved);
    connect(fact,&Fact::itemRemoved,this,&MissionListModel::itemRemoved);
  }
}
//=============================================================================
void MissionListModel::itemToBeInserted(int row, FactTree *)
{
  Fact *fact=qobject_cast<Fact*>(sender());
  if(!fact)return;
  row+=sectionRow(fact);
  beginInsertRows(QModelIndex(), row ,row);
}
void MissionListModel::itemInserted(FactTree *)
{
  endInsertRows();
}
void MissionListModel::itemToBeRemoved(int row,FactTree *)
{
  Fact *fact=qobject_cast<Fact*>(sender());
  if(!fact)return;
  row+=sectionRow(fact);
  beginRemoveRows(QModelIndex(), row ,row);
}
void MissionListModel::itemRemoved(FactTree *)
{
  endRemoveRows();
}
//=============================================================================
int MissionListModel::sectionRow(Fact *fact) const
{
  int srow=0;
  for(int i=0;i<groups.indexOf(fact);++i){
    srow+=groups.at(i)->size();
  }
  return srow;
}
//=============================================================================
QList<FactTree*> MissionListModel::items() const
{
  QList<FactTree*> list;
  for(int i=0;i<groups.size();++i){
    list.append(groups.at(i)->childItems());
  }
  return list;
}
//=============================================================================
//=============================================================================
int MissionListModel::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  int sz=0;
  for(int i=0;i<groups.size();++i){
    sz+=groups.at(i)->size();
  }
  return sz;
}
//=============================================================================
QHash<int, QByteArray> MissionListModel::roleNames() const
{
  return mission->model()->roleNames();
}
//=============================================================================
QVariant MissionListModel::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= rowCount())
    return QVariant();
  Fact *item=static_cast<Fact*>(items().at(index.row()));
  return item->data(index.column(),role);
}
//=============================================================================
