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
#include "ValueEditorArray.h"
//=============================================================================
ValueEditorArray::ValueEditorArray(MissionItemCategoryBase *group, QWidget *parent)
  :ValueEditor(group,parent),group(group)
{
}
ValueEditorArray::~ValueEditorArray()
{
}
//=============================================================================
QWidget * ValueEditorArray::getWidget(QWidget *parent)
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

  model=new ValueEditorArrayModel(group,parent);
  treeView->setModel(model);
  treeView->setItemDelegate(group->model->delegate);
  //treeView->setSelectionModel(group->model->selectionModel);

  treeView->setSelectionBehavior(QAbstractItemView::SelectItems);

  connect(this,SIGNAL(upload()),group->model,SLOT(upload()));

  return treeView;
}
//=============================================================================
//=============================================================================
ValueEditorArrayModel::ValueEditorArrayModel(MissionItemCategoryBase *group, QObject *parent)
  :QAbstractItemModel(parent),group(group)
{
}
//=============================================================================
QVariant ValueEditorArrayModel::data(const QModelIndex &index, int role) const
{
  int col=index.column();
  uint item_n=index.row();
  if(col==0) return group->child(item_n)->data(MissionItem::tc_field,role);
  if(role==Qt::ToolTipRole)return QVariant();
  MissionItemField *f=field(index);
  if(!f)return QVariant();
  return f->data(MissionItem::tc_value,role);
}
//=============================================================================
bool ValueEditorArrayModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role!=Qt::EditRole || (!index.internalPointer()))return false;
  MissionItemField *f=field(index);
  if(!f)return false;
  bool rv=f->setData(MissionItem::tc_value,value);
  emit layoutChanged();
  return rv;
}
//=============================================================================
MissionItemField * ValueEditorArrayModel::field(const QModelIndex &index) const
{
  return static_cast<MissionItemField*>(index.internalPointer());
}
//=============================================================================
QModelIndex ValueEditorArrayModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))return QModelIndex();
  if (parent.isValid()) //root
    return QModelIndex();
  if(column<1 || row>=group->childCount())return createIndex(row,column);
  return createIndex(row,column,group->child(row)->childItemsFlat().at(column-1));
}
//=============================================================================
QModelIndex ValueEditorArrayModel::parent(const QModelIndex &child) const
{
  Q_UNUSED(child)
  return QModelIndex();
}
//=============================================================================
int ValueEditorArrayModel::rowCount(const QModelIndex &parent) const
{
  if(parent.isValid())return 0;
  return group->childCount();
}
//=============================================================================
int ValueEditorArrayModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  if(parent.isValid() || (!group->childCount()))return 0;
  return group->child(0)->childItemsFlat().size()+1;
}
//=============================================================================
QVariant ValueEditorArrayModel::headerData(int section, Qt::Orientation orientation,int role) const
{
  Q_UNUSED(orientation);
  if(section==0)return QVariant("#");
  section--;
  if(!group->childCount())return QVariant();

  if (role==Qt::DisplayRole)
    return group->child(0)->childItemsFlat().at(section)->caption();
  if (role==Qt::ToolTipRole)
    return group->child(0)->childItemsFlat().at(section)->descr();
  return QVariant();
}
//=============================================================================
Qt::ItemFlags ValueEditorArrayModel::flags(const QModelIndex & index) const
{
  if(index.column()>0) return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
  return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}
//=============================================================================
