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
#include "FactDelegateArray.h"
//=============================================================================
FactDelegateArray::FactDelegateArray(Fact *fact, QWidget *parent)
  :FactDelegateDialog(fact,parent)
{
  treeView = new QTreeView(parent);
  treeView->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  treeView->setAlternatingRowColors(true);
  treeView->setRootIsDecorated(false);
  treeView->setEditTriggers(treeView->editTriggers()|QAbstractItemView::SelectedClicked);
  treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  treeView->header()->setMinimumSectionSize(50);
  treeView->setAutoFillBackground(false);

  model=new FactDelegateArrayModel(fact,parent);
  treeView->setModel(model);
  treeView->setItemDelegate(new FactDelegate(parent));

  setWidget(treeView);
}
//=============================================================================
//=============================================================================
FactDelegateArrayModel::FactDelegateArrayModel(Fact *group, QObject *parent)
  :QAbstractItemModel(parent),group(group)
{
}
//=============================================================================
QVariant FactDelegateArrayModel::data(const QModelIndex &index, int role) const
{
  /*int col=index.column();
  uint item_n=index.row();
  bool binded=true;
  Fact *bind_item=static_cast<Fact*>(group->child(0));
  if(bind_item->ftype==ft_varmsk){
    if(bind_item->child(item_n)->isZero()) binded=false;
  }else bind_item=NULL;*/


  /*switch(role){
    case Qt::ForegroundRole:{
      //check if binding exists & enabled
      if(col==0){
        //highlight binded numbers
        //if(bind_item) return binded?QColor(Qt::darkCyan):QColor(Qt::gray);
        return QColor(Qt::cyan).lighter(180);
      }
      if(bind_item && (!binded))return QColor(Qt::darkGray);
      Fact *f=field(index);
      if(!f)return QVariant();
      //zero values are gray
      if(f->ftype==ft_varmsk || f->field->conf_name.startsWith("ctr_ch[")){
        //check if same vars binded to other numbers
        if(match(index,Qt::DisplayRole,data(index,Qt::DisplayRole),2,Qt::MatchFlags(Qt::MatchExactly|Qt::MatchWrap)).size()>1)
          return QColor(Qt::blue).lighter(180);
        else return QColor(Qt::cyan);
      }
      if(f->isModified()) return QColor(Qt::red).lighter();
      if(f->isZero())return QColor(Qt::darkGray);
      return QVariant();
    }
    case Qt::FontRole:
      if(showNum&&col==0) return QFont("",-1,QFont::Bold);
      return QVariant();
    default:
      if(role==Qt::DisplayRole||role==Qt::EditRole) break;
      return QVariant();
  }*/
  if(index.column()==0){
    while(role==Qt::DisplayRole){
      Fact *f=qobject_cast<Fact*>(group->child(0));
      if(!f)break;
      f=qobject_cast<Fact*>(f->child(index.row()));
      if(!f)break;
      return f->title();
      //return index.row()+1;
    }
    return QVariant();
  }

  if(!index.internalPointer())return QVariant();
  Fact *f=field(index);
  if(!f)return QVariant();

  //if(col>0 && bind_item && (!binded) && f->isZero())return QVariant();

  /*QString su=f->field->units();
  if(f->field->conf_name.startsWith("ctr_ch_") && su=="%"){
    int vi=v.toInt();
    if(role!=Qt::DisplayRole)return vi;
    return v>0?QString("+%1").arg(vi):QString("%1").arg(vi);
  }*/
  return f->data(Fact::FACT_MODEL_COLUMN_VALUE,role);
}
//=============================================================================
bool FactDelegateArrayModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role!=Qt::EditRole)return false;
  Fact *f=field(index);
  if(!f)return false;
  if(!f->setValue(value))return false;
  emit layoutChanged();
  return true;
  //connect(f->field,SIGNAL(changed()),this,SIGNAL(layoutChanged()));
}
//=============================================================================
Fact *FactDelegateArrayModel::field(const QModelIndex &index) const
{
  Fact *f=qobject_cast<Fact*>(static_cast<QObject*>(index.internalPointer()));
  if(!f)return NULL;
  int item_n=index.row();
  //check for controls array
  if(f->name().startsWith("ctr_")){
    QString s=f->name();
    s=s.left(s.lastIndexOf('_'));
    //xx_yy_zz nested array
    if(s.contains('_')){
      foreach(FactTree *i,group->childItems()){
        Fact *fi=qobject_cast<Fact*>(i);
        if(!fi->name().startsWith(s))continue;
        if((int)item_n>=fi->size())break; //protect
        fi=qobject_cast<Fact*>(fi->child(item_n));
        if(fi)item_n=fi->value().toInt();
        break;
      }
    }
  }
  if(item_n>=f->size())return NULL;
  return qobject_cast<Fact*>(f->child(item_n));
}
//=============================================================================
QModelIndex FactDelegateArrayModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))return QModelIndex();
  if (parent.isValid()) //root
    return QModelIndex();
  if(column<1)return createIndex(row,column);
  return createIndex(row,column,group->child(column-1));
}
//=============================================================================
QModelIndex FactDelegateArrayModel::parent(const QModelIndex &child) const
{
  Q_UNUSED(child)
  return QModelIndex();
}
//=============================================================================
int FactDelegateArrayModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid())return 0;
  Fact *f=qobject_cast<Fact*>(group->child(0));
  return f?f->size():0;
}
//=============================================================================
int FactDelegateArrayModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return group->size()+1;
}
//=============================================================================
QVariant FactDelegateArrayModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  Q_UNUSED(orientation);
  if(section==0)return QVariant("#");
  Fact *f=qobject_cast<Fact*>(group->child(section-1));
  if(!f) return QVariant();
  if (role==Qt::DisplayRole){
    QString s=f->title();
    return s.contains('_')?s.mid(s.lastIndexOf('_')+1):s;
  }
  if (role==Qt::ToolTipRole) return static_cast<Fact*>(group->child(section-1))->descr();
  return QVariant();
}
//=============================================================================
Qt::ItemFlags FactDelegateArrayModel::flags(const QModelIndex & index) const
{
  const Qt::ItemFlags f=Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
  const Qt::ItemFlags f0=Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  int column=index.column();
  if(column==0)return f0;
  return f;
}
//=============================================================================
