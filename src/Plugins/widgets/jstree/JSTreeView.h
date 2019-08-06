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
#ifndef JSTreeView_H
#define JSTreeView_H
//=============================================================================
#include <TreeModel/JSTreeModel.h>
#include <QtCore>
#include <QtWidgets>
//=============================================================================
class JSTreeView : public QTreeView
{
    Q_OBJECT
public:
    JSTreeView(QWidget *parent = 0);
};
//=============================================================================
class JSTreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    JSTreeProxyModel(QObject *parent = nullptr);
    void setRootItem(JSTreeItem *jsItem);
    JSTreeItem *rootItem() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan2(const QModelIndex &left, const QModelIndex &right) const;

private:
    QPointer<JSTreeItem> m_rootItem;
    bool showThis(const QModelIndex index) const;
};
//=============================================================================
class JSTreeWidget : public QWidget
{
    Q_OBJECT
public:
    JSTreeWidget(QJSEngine *e, bool filterEdit, bool backNavigation, QWidget *parent = 0);

    JSTreeView *tree;
    JSTreeProxyModel *proxy;
    JSTreeModel *model;
    QLineEdit *eFilter;
    QAction *aBack;
    QLabel *lbPath;
    QToolBar *toolBar;
    QVBoxLayout *vlayout;

    JSTreeItem *rootItem() const { return proxy->rootItem(); }

private:
    QList<QPointer<JSTreeItem>> rootList;
private slots:
    void filterChanged();
    void doubleClicked(const QModelIndex &index);
    void collapsed(const QModelIndex &index);
    void expanded(const QModelIndex &index);
    void updateActions();
    void jsItemRemoved();

public slots:
    void resetFilter();
    void back();
    void setRoot(JSTreeItem *jsItem);

signals:
    void treeReset();
};
//=============================================================================
#endif
