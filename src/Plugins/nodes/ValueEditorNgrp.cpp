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
#include "ValueEditorNgrp.h"
//=============================================================================
ValueEditorNgrp::ValueEditorNgrp(NodesItem *root, QWidget *parent)
  :ValueEditor(root,parent)
{
}
ValueEditorNgrp::~ValueEditorNgrp()
{
}
//=============================================================================
QWidget * ValueEditorNgrp::getWidget(QWidget *parent)
{
  treeView = new QTreeView(parent);
  QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  treeView->setSizePolicy(sizePolicy);

  treeView->setAlternatingRowColors(true);
  treeView->setRootIsDecorated(false);
  treeView->setEditTriggers(treeView->editTriggers()|QAbstractItemView::SelectedClicked);
  treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  treeView->header()->setMinimumSectionSize(50);
  treeView->setAutoFillBackground(false);

  model=new ValueEditorNgrpModel(field,parent);
  treeView->setModel(model);
  treeView->setItemDelegate(new NodesItemDelegate(parent));

  return treeView;
}
//=============================================================================
//=============================================================================
ValueEditorNgrpModel::ValueEditorNgrpModel(NodesItem *root, QObject *parent)
  :QAbstractItemModel(parent),root(root)
{
  //populate consolidated fnames list
  int nidx=-1;
  int fidx=0;
  foreach(NodesItem *i,root->childItems){
    nidx++;
    if(i->item_type!=NodesItem::it_node)continue;
    NodesItemNode *node=static_cast<NodesItemNode*>(i);
    foreach(NodesItemField *f,node->fields){
      QString s=f->conf_name;
      if(f->childCount()){
        int cidx=0;
        foreach(NodesItem *fi,f->childItems){
          QString sf;
          if(f->array>1)sf=QString("%1/%2").arg(s.left(s.indexOf('['))).arg(QString::number(cidx++));
          else sf=QString("%1/%2").arg(s.indexOf('[')>0?s.left(s.indexOf('[')):s).arg(fi->name);
          if(!fnames.contains(sf)){
            fnames.insert(fidx,sf);
            fdescr.append(fi->descr.isEmpty()?f->descr:fi->descr);
          }
          fidx=fnames.indexOf(sf)+1;
          map.insert(QString("%1:%2").arg(sf).arg(nidx),static_cast<NodesItemField*>(fi));
        }
        continue;
      }
      if(!fnames.contains(s)){
        fnames.insert(fidx,s);
        fdescr.append(f->descr);
      }
      fidx=fnames.indexOf(s)+1;
      map.insert(QString("%1:%2").arg(s).arg(nidx),f);
    }
  }
}
//=============================================================================
QVariant ValueEditorNgrpModel::data(const QModelIndex &index, int role) const
{
  NodesItemField *f=field(index);
  if(!f)return QVariant();
  if(role==NodesModel::NodesItemRole) return QVariant::fromValue<NodesItem*>(static_cast<NodesItem*>(f));
  if(role==Qt::ToolTipRole) return data(this->index(index.row(),0,QModelIndex()),Qt::DisplayRole);
  if(f->isModified() && role!=Qt::DisplayRole && role!=Qt::EditRole)
    return f->data(NodesItem::tc_field,role);
  return f->data(NodesItem::tc_value,role);
}
//=============================================================================
bool ValueEditorNgrpModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if(role!=Qt::EditRole)return false;
  NodesItemField *f=field(index);
  if(!f)return false;
  return f->setValue(value);
}
//=============================================================================
NodesItemField * ValueEditorNgrpModel::field(const QModelIndex &index) const
{
  if(!index.internalPointer())return NULL;
  return static_cast<NodesItemField*>(index.internalPointer());
}
//=============================================================================
QHash<QString, NodesItemField *> ValueEditorNgrpModel::getFieldsHash() const
{
  //plain list of all unique field names
  QHash<QString, NodesItemField *> hash;
  foreach(NodesItem *i,root->childItems){
    if(i->item_type!=NodesItem::it_node)continue;
    NodesItemNode *node=static_cast<NodesItemNode*>(i);
    foreach(NodesItemField *f,node->fields){
      hash.insertMulti(f->conf_name,f);
    }
  }
  return hash;
}
//=============================================================================
QModelIndex ValueEditorNgrpModel::index(int row, int column, const QModelIndex &parent) const
{
 // if (!hasIndex(row, column, parent))return QModelIndex();
  if (parent.isValid()) //root
    return QModelIndex();
  if(row>=root->childCount()||column>=fnames.size()||root->child(row)->item_type!=NodesItem::it_node)
    return createIndex(row,column);
  //return map.value(QString("%1_%2").arg(fnames.at(column)).arg(row))->model_index;
  return createIndex(row,column,map.value(QString("%1:%2").arg(fnames.at(column)).arg(row)));
}
//=============================================================================
QModelIndex ValueEditorNgrpModel::parent(const QModelIndex &child) const
{
  Q_UNUSED(child)
  return QModelIndex();
}
//=============================================================================
int ValueEditorNgrpModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid() || (!root->childCount()))return 0;
  return root->childCount();
}
//=============================================================================
int ValueEditorNgrpModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return fnames.size();
}
//=============================================================================
QVariant ValueEditorNgrpModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  Q_UNUSED(orientation);
  if(role==Qt::FontRole)return QFont("",-1,QFont::Bold,false);
  if(role==Qt::ToolTipRole)return section<fdescr.size()?fdescr.at(section):QVariant();
  if(role!=Qt::DisplayRole)return QVariant();
  if(section>=fnames.size())return QVariant();
  return fnames.at(section);
}
//=============================================================================
Qt::ItemFlags ValueEditorNgrpModel::flags(const QModelIndex & index) const
{
  NodesItemField *f=field(index);
  if(!f)return Qt::ItemIsEnabled;
  return f->flags(NodesItem::tc_value);
}
//=============================================================================
