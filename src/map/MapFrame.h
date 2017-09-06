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
#ifndef MAPFRAME_H
#define MAPFRAME_H
//=============================================================================
#include "ui_MapFrame.h"
#include <QtCore>
#include "MissionModel.h"
#include "MissionListView.h"
//=============================================================================
class MapFrame: public QWidget, public Ui::MapFrame
{
  Q_OBJECT
public:
  MapFrame(QWidget *parent = 0);
  bool aboutToQuit();
  MissionModel *model;
private:
  QMandala *mandala;
  MissionListView *missionListView;

  Qt::WindowFlags wf_save;
  bool df_save;
  QByteArray geometry_save;
  QAction *aUavShowHdg;

private slots:
  void mandalaCurrentChanged(QMandalaItem *m);
  void updateStats();
  void modelSelectionChanged();
  void mapItemClicked(MapTile *item);
  void listPointClicked(MissionItemObject *item);

  void mapToolTriggered(bool);

  void on_aZoomIn_triggered();
  void on_aZoomOut_triggered();

  void on_aFullScreen_triggered(void);

  void on_aTraceReset_triggered();
  void on_aTraceResetF_triggered();
  void on_aTraceShow_toggled(bool checked);

  void on_aSetHome_triggered();
  void on_aFlyHere_triggered();
  void on_aLookHere_triggered();
  void on_aPosFix_triggered();

  void on_aWpAdj_triggered();
  void on_aLand_triggered();
  void on_aGoWpt_triggered();
  void on_aGoPi_triggered();

  void aUavShowHdg_triggered(bool checked);

  //files
  bool saveChanges(void);
  void on_aSave_triggered();
  void on_aSaveAs_triggered();
  void on_aNewFile_triggered();
  void on_aLoad_triggered();
};
//=============================================================================
#endif
