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
#ifndef NODESVIEW_H
#define NODESVIEW_H
//=============================================================================
#include <QWidget>
#include "NodesModel.h"
#include <QStyledItemDelegate>
//=============================================================================
class NodesTreeView : public QTreeView
{
  Q_OBJECT
public:
  NodesTreeView(QWidget *parent = 0);
};
//=============================================================================
class NodesItemDelegate : public QItemDelegate//QStyledItemDelegate
{
  Q_OBJECT
public:
  NodesItemDelegate(QObject *parent = 0);
  ~NodesItemDelegate();
  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
  void setEditorData(QWidget *editor, const QModelIndex &index) const;
  void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
protected:
  QWidget *createEditorEx(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index,NodesItemField *f) const;
  void paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const;
private:
  QProgressBar *progressBar;
  bool drawProgress(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index,uint progress) const;
};
//=============================================================================
class NodesSortFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  NodesSortFilterProxyModel(QObject *parent = 0):QSortFilterProxyModel(parent){}
protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;
private:
  static QStringList sortNames;
  bool showThis(const QModelIndex index) const;
};
//=============================================================================
#endif
