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
#ifndef NODESFRAME_H
#define NODESFRAME_H
//=============================================================================
#include <QWidget>
#include "ui_NodesFrame.h"
#include "NodesModel.h"
#include "NodesView.h"
class QMandala;
//=============================================================================
class NodesFrame : public QWidget, public Ui::NodesFrame
{
  Q_OBJECT
public:
  NodesFrame(QWidget *parent = 0);

private:
  QMandala *mandala;
  QToolBar *toolBar;
  QToolButton *btnUpload;
  NodesModel *model;
  QHash<QMandalaItem*,NodesModel*>models;
  NodesItemDelegate delegate;
  NodesSortFilterProxyModel proxy;

  QList<NodesItem*> selectedItems(NodesItem::_item_type item_type=NodesItem::it_root) const;

  uint progressCnt;

  QTimer updateActionsTimer;

private slots:
  void currentMandalaChanged(QMandalaItem *m);
  void mandalaSizeChanged(uint sz);

  void on_lbUavName_clicked();

  void on_aRequest_triggered(void);
  void on_aUpload_triggered(void);
  void on_aSave_triggered(void);
  void on_aLoad_triggered(void);
  void on_aUndo_triggered(void);
  void on_aReload_triggered(void);
  void on_aStats_triggered(void);
  void on_aLoadTelemetry_triggered(void);
  void on_tree_customContextMenuRequested(const QPoint &pos);

  void syncProgress(uint cnt);

  void nodeCmdAction(void);
  void nodeRestoreBackup(void);
  void vehicleRestoreBackup(void);
  void nodeRestoreRecentBackup(void);
  void nodeRebootAll(void);

  void nodeUpdateFirmware(void);

  void updateActions(void);
  void updateActionsDo(void);

  void filterChanged();

  void resetTree();

};
//=============================================================================
#endif // CTRFRAME_H
