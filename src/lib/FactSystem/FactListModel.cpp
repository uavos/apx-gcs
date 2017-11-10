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
#include "Fact.h"
#include "FactListModel.h"
//=============================================================================
FactListModel::FactListModel(Fact *parent)
 : QAbstractListModel(parent),
   fact(parent),
   m_flat(false)
{

  connect(fact,&Fact::itemToBeInserted,[=](int row, FactTree*){
    beginInsertRows(QModelIndex(),row,row);
  });
  connect(fact,&Fact::itemInserted,[=](FactTree*){
    endInsertRows();
  });

  connect(fact,&Fact::itemToBeRemoved,[=](int row, FactTree*){
    beginRemoveRows(QModelIndex(),row,row);
  });
  connect(fact,&Fact::itemRemoved,[=](FactTree*){
    endRemoveRows();
  });
}
//=============================================================================
Fact * FactListModel::flatParent() const
{
  if(fact->treeItemType()!=Fact::SectionItem)return NULL;
  Fact *fParent=NULL;
  for(FactTree *i=fact->parentItem();i;i=i->parentItem()){
    Fact *f=static_cast<Fact*>(i);
    if(!f->model()->flat())break;
    fParent=f;
  }
  return fParent;
}
//=============================================================================
//=============================================================================
int FactListModel::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  return fact->size();
}
//=============================================================================
QHash<int, QByteArray> FactListModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[ModelDataRole]  = "modelData";
  roles[NameRole]       = "name";
  roles[ValueRole]      = "value";
  roles[TextRole]       = "text";
  return roles;
}
//=============================================================================
QVariant FactListModel::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= fact->size())
    return QVariant();
  Fact *item=static_cast<Fact*>(fact->child(index.row()));
  switch(role){
    case ModelDataRole: return QVariant::fromValue(item);
    case NameRole:      return item->name();
    case ValueRole:     return item->value();
    case TextRole:      return item->text();
  }
  return QVariant();
}
//=============================================================================
bool FactListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (index.row() < 0 || index.row() >= fact->size() || role != Qt::EditRole)
    return false;
  return static_cast<Fact*>(fact->child(index.row()))->setValue(value);
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
