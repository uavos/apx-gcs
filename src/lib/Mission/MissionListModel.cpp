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
#include "MissionGroup.h"
//=============================================================================
MissionListModel::MissionListModel(VehicleMission *parent)
 : QAbstractListModel(parent),
   mission(parent)
{
  foreach (Fact *fact, mission->groups) {
    connect(fact,&Fact::itemToBeInserted,this,&MissionListModel::itemToBeInserted);
    connect(fact,&Fact::itemInserted,this,&MissionListModel::itemInserted);

    connect(fact,&Fact::itemToBeRemoved,this,&MissionListModel::itemToBeRemoved);
    connect(fact,&Fact::itemRemoved,this,&MissionListModel::itemRemoved);

    connect(fact,&Fact::itemToBeMoved,this,&MissionListModel::itemToBeMoved);
    connect(fact,&Fact::itemMoved,this,&MissionListModel::itemMoved);
  }
}
//=============================================================================
void MissionListModel::itemToBeInserted(int row, FactTree *)
{
  MissionGroup *fact=qobject_cast<MissionGroup*>(sender());
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
  MissionGroup *fact=qobject_cast<MissionGroup*>(sender());
  if(!fact)return;
  row+=sectionRow(fact);
  beginRemoveRows(QModelIndex(), row ,row);
}
void MissionListModel::itemRemoved(FactTree *)
{
  endRemoveRows();
}
void MissionListModel::itemToBeMoved(int row,int dest,FactTree *)
{
  MissionGroup *fact=qobject_cast<MissionGroup*>(sender());
  if(!fact)return;
  int r=sectionRow(fact);
  row+=r;
  dest+=r;
  beginMoveRows(QModelIndex(), row ,row, QModelIndex(), dest);
}
void MissionListModel::itemMoved(FactTree *)
{
  endMoveRows();
}
//=============================================================================
int MissionListModel::sectionRow(MissionGroup *fact) const
{
  int srow=0;
  for(int i=0;i<mission->groups.indexOf(fact);++i){
    srow+=mission->groups.at(i)->size();
  }
  return srow;
}
//=============================================================================
QList<FactTree*> MissionListModel::items() const
{
  QList<FactTree*> list;
  for(int i=0;i<mission->groups.size();++i){
    list.append(mission->groups.at(i)->childItems());
  }
  return list;
}
//=============================================================================
//=============================================================================
int MissionListModel::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  int sz=0;
  for(int i=0;i<mission->groups.size();++i){
    sz+=mission->groups.at(i)->size();
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
