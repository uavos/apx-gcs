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
#include <QDomDocument>
#include <FactSystem.h>
#include "FactTreeModel.h"
//=============================================================================
FactTreeModel::FactTreeModel(Fact *root)
 : QAbstractItemModel(root),
   root(root)
{
  connect(root,&Fact::itemToBeAdded,[=](int i,FactTree *item){
    beginInsertRows(factIndex(item->parentItem()),i,i);
  });
  connect(root,&Fact::itemAdded,[=](){
    endInsertRows();
  });
  connect(root,&Fact::itemToBeRemoved,[=](int i,FactTree *item){
    beginRemoveRows(factIndex(item->parentItem()),i,i);
  });
  connect(root,&Fact::itemRemoved,[=](){
    endRemoveRows();
  });
}
//=============================================================================
//=============================================================================
QVariant FactTreeModel::data(const QModelIndex &index, int role) const
{
  if(!index.isValid()) return QVariant();
  Fact *i=fact(index);
  if(!i)return QVariant();
  if(role==Fact::ModelDataRole) return QVariant::fromValue<Fact*>(i);

  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(index.column()){
    case FACT_MODEL_COLUMN_NAME: return i->title();
    case FACT_MODEL_COLUMN_VALUE: return i->text();
    case FACT_MODEL_COLUMN_DESCR: return i->descr();
    default: return QVariant();
  }
}
//=============================================================================
bool FactTreeModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if ((!index.isValid()) || (role!=Qt::EditRole) || index.column()!=FACT_MODEL_COLUMN_VALUE)
    return false;
  Fact *i = fact(index);
  if(!i)return false;
  if (data(index,role)==value)return true;
  bool rv=i->setValue(value);
  //if(rv)emit dataChanged(index,index);//layoutChanged();
  return rv;
}
//=============================================================================
//=============================================================================
QModelIndex FactTreeModel::index(int row, int column, const QModelIndex &parent) const
{
  if(!hasIndex(row, column, parent)) return QModelIndex();
  Fact *parentFact;
  if(!parent.isValid()) parentFact = root;
  else parentFact = fact(parent);
  if(!parentFact) return QModelIndex();
  Fact *childFact = static_cast<Fact*>(parentFact->childItemsTree().at(row));
  if(!childFact) return QModelIndex();
  //QModelIndex i=createIndex(row, column, childFact);
  checkConnections(childFact);
  return factIndex(childFact,column);
}
//=============================================================================
QModelIndex FactTreeModel::parent(const QModelIndex &index) const
{
  if(!index.isValid()) return QModelIndex();
  Fact *i=fact(index);
  if(!i)return QModelIndex();
  Fact *parentFact = static_cast<Fact*>(i->parentItem());
  if (!parentFact || parentFact == root) return QModelIndex();
  //QModelIndex parentIndex=createIndex(parentFact->num(),0,parentFact);
  //parentFact->model_index=parentIndex;
  checkConnections(parentFact);
  return factIndex(parentFact);
}
//=============================================================================
int FactTreeModel::rowCount(const QModelIndex &parent) const
{
  Fact *parentFact;
  if (parent.column() > 0) return 0;
  if (!parent.isValid()) parentFact = root;
  else parentFact = fact(parent);
  //if(parentFact->treeItemType()==Fact::GroupItem && static_cast<FactGroup*>(parentItem)->isValueArray())return 0;
  //qDebug()<<parentFact;
  return parentFact->childItemsTree().size();
}
//=============================================================================
int FactTreeModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent)
  return FACT_MODEL_COLUMN_CNT;
}
//=============================================================================
QVariant FactTreeModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  Q_UNUSED(orientation)
  if(role==Qt::DisplayRole){
    switch(section){
      case FACT_MODEL_COLUMN_NAME: return tr("Name");
      case FACT_MODEL_COLUMN_VALUE: return tr("Value");
      case FACT_MODEL_COLUMN_DESCR: return tr("Description");
    }
  }
  return QVariant();
}
//=============================================================================
Qt::ItemFlags FactTreeModel::flags(const QModelIndex & index) const
{
  Qt::ItemFlags f=Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  Fact *i=fact(index);
  if(i && i->treeItemType()==Fact::FactItem){
    f|=Qt::ItemNeverHasChildren;
    if(index.column()==FACT_MODEL_COLUMN_VALUE) f|=Qt::ItemIsEditable;
  }
  return f;
}
//=============================================================================
Fact * FactTreeModel::fact(const QModelIndex &index) const
{
  return static_cast<Fact*>(index.internalPointer());
}
QModelIndex FactTreeModel::factIndex(FactTree * item, int column) const
{
  return createIndex(item->num(),column,item);
  /*foreach (QModelIndex index, persistentIndexList()) {
    if(index.column()!=column)continue;
    Fact * f=fact(index);
    if(f==item)return index;
  }
  if(doCreateIndex) return createIndex(item->num(),column,item);
  return QModelIndex();*/
}
//=============================================================================
void FactTreeModel::checkConnections(Fact *fact) const
{
  connect(fact,&Fact::textChanged, this, &FactTreeModel::factTextChanged ,Qt::UniqueConnection);
  connect(fact,&Fact::titleChanged, this, &FactTreeModel::factTitleChanged ,Qt::UniqueConnection);
  connect(fact,&Fact::descrChanged, this, &FactTreeModel::factDescrChanged ,Qt::UniqueConnection);
}
//=============================================================================
//=============================================================================
void FactTreeModel::factTextChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_VALUE);
  dataChanged(index,index,QVector<int>()<<Qt::DisplayRole);
  //qDebug()<<index;
}
void FactTreeModel::factTitleChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_NAME);
  dataChanged(index,index,QVector<int>()<<Qt::DisplayRole);
}
void FactTreeModel::factDescrChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_DESCR);
  dataChanged(index,index,QVector<int>()<<Qt::DisplayRole);
}
//=============================================================================
