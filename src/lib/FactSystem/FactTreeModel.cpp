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
  updateTimer.setSingleShot(true);
  updateTimer.setInterval(100);
  connect(&updateTimer,&QTimer::timeout,this,&FactTreeModel::updateTimerTimeout);
}
//=============================================================================
//=============================================================================
QVariant FactTreeModel::data(const QModelIndex &index, int role) const
{
  if(!index.isValid()) return QVariant();
  Fact *f=fact(index);
  if(!f)return QVariant();
  if(role==FactListModel::ModelDataRole) return QVariant::fromValue<Fact*>(f);

  switch(role){
  case Qt::ForegroundRole:
    if(index.column()==FACT_MODEL_COLUMN_NAME){
      if(f->modified())return QColor(Qt::red).lighter();
      if(!f->enabled())return QColor(Qt::gray);
      if(f->active())return QColor(Qt::green).lighter();
      if(f->treeItemType()==Fact::FactItem) return QColor(Qt::gray).lighter(150);
      return QVariant();
    }
    if(index.column()==FACT_MODEL_COLUMN_VALUE){
      if(!f->enabled())return QColor(Qt::darkGray);
      if(f->dataType()==Fact::ActionData) return QColor(Qt::blue).lighter(170);
      if(f->size()) return QColor(Qt::darkGray); //expandable
      //if(ftype==ft_string) return QVariant();
      //if(ftype==ft_varmsk) return QColor(Qt::cyan);
      return QColor(Qt::cyan).lighter(180);
    }
    return QColor(Qt::darkCyan);
  case Qt::BackgroundRole:
    return QVariant();
  case Qt::FontRole:
    if(index.column()==FACT_MODEL_COLUMN_DESCR) return QVariant();
    if(f->treeItemType()!=Fact::FactItem) return QFont("",0,QFont::Bold,false);
    //if(ftype>=ft_regPID) return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
    //if(index.column()==FACT_MODEL_COLUMN_NAME) return QFont("Monospace",-1,QFont::Normal,isModified());
    //if(ftype==ft_string) return QFont("",-1,QFont::Normal,true);
    return QVariant();
  case Qt::ToolTipRole:
    /*if(index.column()==tc_field) return conf_name;
    else if(index.column()==tc_value){
      if(childItems.size()){
        QString s=name;
        foreach(NodesItem *i,childItems)
          s+=QString("\n%1: %2").arg(i->name).arg(i->data(tc_value,Qt::DisplayRole).toString());
        return s;
      }else return data(tc_descr,Qt::DisplayRole);
    }*/
    return data(index,Qt::DisplayRole);
  }

  //value roles
  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(index.column()){
    case FACT_MODEL_COLUMN_NAME: return f->title();
    case FACT_MODEL_COLUMN_VALUE:{
      if(f->dataType()==Fact::ActionData) return QString("<exec>");
      const QString s=f->text();
      if(s.isEmpty()) return f->status();
      return s;
    }
    case FACT_MODEL_COLUMN_DESCR: return f->descr();
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
  Fact *childFact = static_cast<Fact*>(parentFact->child(row));
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
  //checkConnections(i);
  if(!i->parentItem())return QModelIndex();
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
  return parentFact->size();
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
    //f|=Qt::ItemNeverHasChildren;
    if(index.column()==FACT_MODEL_COLUMN_VALUE &&
       i->enabled() &&
       i->dataType()!=Fact::NoData &&
       i->dataType()!=Fact::ConstData
       ) f|=Qt::ItemIsEditable;
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
}
//=============================================================================
void FactTreeModel::checkConnections(Fact *fact) const
{
  if(fact->treeModelSync)return;

  connect(fact,&Fact::destroyed,this,&FactTreeModel::itemDestroyed);

  //if(fact->path().contains("nodes.node"))qDebug()<<fact->path()<<fact;
  connect(fact,&Fact::itemToBeInserted,this,&FactTreeModel::itemToBeInserted);
  connect(fact,&Fact::itemInserted,this,&FactTreeModel::itemInserted);
  connect(fact,&Fact::itemToBeRemoved,this,&FactTreeModel::itemToBeRemoved);
  connect(fact,&Fact::itemRemoved,this,&FactTreeModel::itemRemoved);

  connect(fact,&Fact::textChanged, this, &FactTreeModel::textChanged );
  connect(fact,&Fact::statusChanged, this, &FactTreeModel::textChanged );
  connect(fact,&Fact::titleChanged, this, &FactTreeModel::titleChanged );
  connect(fact,&Fact::descrChanged, this, &FactTreeModel::descrChanged );
  connect(fact,&Fact::enabledChanged, this, &FactTreeModel::enabledChanged );
  connect(fact,&Fact::activeChanged, this, &FactTreeModel::activeChanged );
  connect(fact,&Fact::progressChanged, this, &FactTreeModel::progressChanged );
  fact->treeModelSync=true;
}
//=============================================================================
void FactTreeModel::itemToBeInserted(int row, FactTree *item)
{
  Fact *fact=static_cast<Fact*>(item->parentItem());
  const QModelIndex &index=factIndex(fact);
  beginInsertRows(index,row,row);
}
void FactTreeModel::itemInserted(FactTree *)
{
  endInsertRows();
}
void FactTreeModel::itemToBeRemoved(int row,FactTree *item)
{
  Fact *fact=static_cast<Fact*>(item->parentItem());
  const QModelIndex &index=factIndex(fact);
  beginRemoveRows(index,row,row);
}
void FactTreeModel::itemRemoved(FactTree *)
{
  endRemoveRows();
}
//=============================================================================
void FactTreeModel::textChanged()
{
  Fact *fact=static_cast<Fact*>(static_cast<Fact*>(sender())->parentItem());
  //qDebug()<<fact->path();
  //QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_VALUE);
  if(!updateList.contains(fact)) updateList.append(fact);
  if(!updateTimer.isActive()) updateTimer.start();
}
void FactTreeModel::titleChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_NAME);
  dataChanged(index,index,QVector<int>()<<Qt::DisplayRole);
}
void FactTreeModel::descrChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_DESCR);
  dataChanged(index,index,QVector<int>()<<Qt::DisplayRole);
}
void FactTreeModel::enabledChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_VALUE);
  dataChanged(index,index,QVector<int>()<<Qt::ForegroundRole);
}
void FactTreeModel::activeChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_NAME);
  dataChanged(index,index,QVector<int>()<<Qt::ForegroundRole);
}
void FactTreeModel::progressChanged()
{
  QModelIndex index=factIndex(static_cast<Fact*>(sender()),FACT_MODEL_COLUMN_DESCR);
  dataChanged(index,index,QVector<int>()<<Qt::DisplayRole);
}
//=============================================================================
void FactTreeModel::updateTimerTimeout()
{
  foreach (Fact *f, updateList) {
    if(!f->size())continue;
    QModelIndex index1=createIndex(0,FACT_MODEL_COLUMN_VALUE,f->childItems().first());
    QModelIndex index2=createIndex(f->size()-1,FACT_MODEL_COLUMN_VALUE,f->childItems().last());
    emit dataChanged(index1,index2,QVector<int>()<<Qt::DisplayRole);
    //qDebug()<<f;
  }
  //emit dataChanged(QModelIndex(),QModelIndex());
  updateList.clear();
}
void FactTreeModel::itemDestroyed()
{
  updateList.removeAll(static_cast<Fact*>(sender()));
}
//=============================================================================
//=============================================================================
