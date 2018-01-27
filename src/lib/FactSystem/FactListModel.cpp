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
#include "FactSystem.h"
#include "FactListModel.h"
//=============================================================================
FactListModel::FactListModel(Fact *parent)
 : QAbstractListModel(parent),
   fact(parent),
   m_flat(false)
{
  if(fact){
    connect(fact,&Fact::itemToBeInserted,this,&FactListModel::itemToBeInserted);
    connect(fact,&Fact::itemInserted,this,&FactListModel::itemInserted);

    connect(fact,&Fact::itemToBeRemoved,this,&FactListModel::itemToBeRemoved);
    connect(fact,&Fact::itemRemoved,this,&FactListModel::itemRemoved);

    connect(fact,&Fact::itemToBeMoved,this,&FactListModel::itemToBeMoved);
    connect(fact,&Fact::itemMoved,this,&FactListModel::itemMoved);
  }
}
//=============================================================================
void FactListModel::itemToBeInserted(int row, FactTree *)
{
  Fact *f=sectionParent(fact);
  if(f!=fact)beginInsertRows(QModelIndex(), row ,row);
  row+=sectionRow(f,fact);
  f->model()->beginInsertRows(QModelIndex(), row ,row);
}
void FactListModel::itemInserted(FactTree *)
{
  Fact *f=sectionParent(fact);
  f->model()->endInsertRows();
  if(f!=fact)endInsertRows();
}
void FactListModel::itemToBeRemoved(int row,FactTree *)
{
  Fact *f=sectionParent(fact);
  if(f!=fact)beginRemoveRows(QModelIndex(), row ,row);
  row+=sectionRow(f,fact);
  f->model()->beginRemoveRows(QModelIndex(), row ,row);
}
void FactListModel::itemRemoved(FactTree *)
{
  Fact *f=sectionParent(fact);
  f->model()->endRemoveRows();
  if(f!=fact)endRemoveRows();
}
void FactListModel::itemToBeMoved(int row,int dest,FactTree *)
{
  Fact *f=sectionParent(fact);
  if(f!=fact)beginMoveRows(QModelIndex(), row ,row, QModelIndex(), dest);
  int r=sectionRow(f,fact);
  row+=r;
  dest+=r;
  f->model()->beginMoveRows(QModelIndex(), row ,row, QModelIndex(), dest);
}
void FactListModel::itemMoved(FactTree *)
{
  Fact *f=sectionParent(fact);
  f->model()->endMoveRows();
  if(f!=fact)endMoveRows();
}
//=============================================================================
Fact * FactListModel::sectionParent(Fact *item) const
{
  if(item->treeItemType()!=Fact::SectionItem)return item;
  Fact *f=static_cast<Fact*>(item->parentItem());
  if(!f->model()->flat())return item;
  return sectionParent(f);
}
int FactListModel::sectionRow(Fact *parent, Fact *sect) const
{
  if(sect->treeItemType()!=Fact::SectionItem)return 0;
  int srow=0;
  foreach(FactTree *i,parent->childItems()){
    Fact *f=static_cast<Fact*>(i);
    if(f->treeItemType()==Fact::SectionItem) {
      if(f==sect)return srow;
      srow+=sectionRow(f,sect);
    }else srow++;
  }
  return srow;
}
//=============================================================================
QList<FactTree*> FactListModel::items() const
{
  if(!m_flat) return fact->childItems();
  QList<FactTree*> list;
  foreach(FactTree *i,fact->childItems()){
    Fact *f=static_cast<Fact*>(i);
    if(f->treeItemType()==Fact::SectionItem) {
      list.append(f->model()->items());
    }else list.append(i);
  }
  return list;
}
//=============================================================================
//=============================================================================
int FactListModel::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  if(!m_flat) return fact->size();
  int sz=0;
  foreach(const FactTree*i,fact->childItems()){
    if(i->treeItemType()==Fact::SectionItem) sz+=static_cast<const Fact*>(i)->model()->rowCount();
    else sz++;
  }
  return sz;
}
//=============================================================================
QHash<int, QByteArray> FactListModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[Fact::ModelDataRole]  = "modelData";
  roles[Fact::NameRole]       = "name";
  roles[Fact::ValueRole]      = "value";
  roles[Fact::TextRole]       = "text";
  return roles;
}
//=============================================================================
QVariant FactListModel::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= rowCount())
    return QVariant();
  Fact *item=static_cast<Fact*>(items().at(index.row()));
  return item->data(index.column(),role);
}
//=============================================================================
bool FactListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (index.row() < 0 || index.row() >= rowCount() || role != Qt::EditRole)
    return false;
  return static_cast<Fact*>(items().at(index.row()))->setValue(value);
}
//=============================================================================
//=============================================================================
bool FactListModel::flat() const
{
  return m_flat;
}
void FactListModel::setFlat(const bool &v)
{
  if(m_flat==v)return;
  m_flat=v;
  emit flatChanged();
}
//=============================================================================
