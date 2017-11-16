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
#ifndef FactTreeView_H
#define FactTreeView_H
//=============================================================================
#include <QtCore>
#include <QWidget>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QToolBar>
class Fact;
class FactTreeModel;
//=============================================================================
class FactTreeView : public QTreeView
{
  Q_OBJECT
public:
  FactTreeView(QWidget *parent = 0);
};
//=============================================================================
class FactProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  FactProxyModel(QObject *parent = 0);

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;
private:
  QStringList sortNames;
  bool showThis(const QModelIndex index) const;
};
//=============================================================================
class FactTreeWidget : public QWidget
{
  Q_OBJECT
public:
  FactTreeWidget(QWidget *parent = 0);

  void setRoot(Fact *fact,bool filterEdit=true,bool backNavigation=true);

  FactTreeView *tree;
  FactProxyModel *proxy;
  FactTreeModel *model;
  QLineEdit *eFilter;
  QAction *aBack;
  QToolBar *toolBar;

private:
  QList<QModelIndex> rootList;
private slots:
  void filterChanged();
  void doubleClicked(const QModelIndex &index);
  void collapsed(const QModelIndex &index);
  void updateActions();

public slots:
  void back();

signals:
  void treeReset();
};
//=============================================================================
#endif
