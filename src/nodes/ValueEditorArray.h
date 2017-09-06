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
#ifndef VALUEEDITORARRAY_H
#define VALUEEDITORARRAY_H
#include "ValueEditor.h"
#include <QAbstractItemModel>
class ValueEditorArrayModel;
//=============================================================================
class ValueEditorArray: public ValueEditor
{
  Q_OBJECT
public:
  explicit ValueEditorArray(NodesItemGroup *group, QWidget *parent = 0);
  ~ValueEditorArray();
  NodesItemGroup *group;
private:
  ValueEditorArrayModel *model;
  QTreeView *treeView;
  QWidget * getWidget(QWidget *parent);
};
//=============================================================================
class ValueEditorArrayModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ValueEditorArrayModel(NodesItemGroup *group,QObject * parent = 0);
  NodesItemGroup *group;
  void emitReset(){emit layoutChanged();}
private:
  bool showNum;
  NodesItemField *field(const QModelIndex &index) const;
  //override
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column=0,const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  Qt::ItemFlags flags(const QModelIndex & index) const;
  bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
};
//=============================================================================
class ValueEditorArrayDelegate : public NodesItemDelegate
{
  Q_OBJECT
public:
  ValueEditorArrayDelegate(QObject *parent = 0) : NodesItemDelegate(parent){}
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
};
//=============================================================================
#endif //VALUEEDITORARRAY_H
