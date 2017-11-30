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
#ifndef NodesFrame_H
#define NodesFrame_H
//=============================================================================
#include <QtWidgets>
#include "ui_NodesFrame.h"
#include "Facts.h"
#include "FactTreeView.h"
//=============================================================================
class NodesFrame : public QWidget, public Ui::NodesFrame
{
  Q_OBJECT
public:
  NodesFrame(QWidget *parent = 0);

private:
  FactTreeWidget *treeWidget;

  QToolBar *toolBar;
  QToolButton *btnUpload;

  template <class T=Fact>
  inline QList<T*> selectedItems() const
  {
    QList<T*> list;
    foreach(QModelIndex index,treeWidget->tree->selectionModel()->selectedRows()){
      Fact *i = index.data(Fact::ModelDataRole).value<Fact*>();
      if(!i)continue;
      T *f=qobject_cast<T*>(i);
      if(f) list.append(f);
    }
    return list;
  }

  Vehicle *vehicle;

private slots:
  void vehicleSelected(Vehicle *v);

  void on_aRequest_triggered(void);
  void on_aReload_triggered(void);
  void on_aUpload_triggered(void);
  void on_aStop_triggered(void);

  void on_aSave_triggered(void);
  void on_aLoad_triggered(void);
  void on_aUndo_triggered(void);
  void on_aStats_triggered(void);
  void on_aLoadTelemetry_triggered(void);
  void on_tree_customContextMenuRequested(const QPoint &pos);

  void nodeCmdAction(void);
  void nodeRestoreBackup(void);
  void vehicleRestoreBackup(void);
  void nodeRestoreRecentBackup(void);
  void nodeRebootAll(void);

  void nodeUpdateFirmware(void);

  void updateActions(void);
};
//=============================================================================
#endif
