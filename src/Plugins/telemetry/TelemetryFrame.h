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
#ifndef TelemetryFrame_H
#define TelemetryFrame_H
//-----------------------------------------------------------------------------
#include "ui_TelemetryFrame.h"
#include "TelemetryPlot.h"
#include "Player.h"
#include <QtCore>
//=============================================================================
class TelemetryFrame: public QWidget, public Ui::TelemetryFrame
{
  Q_OBJECT
public:
  TelemetryFrame(QWidget *parent = 0);
  ~TelemetryFrame();
protected:
  void closeEvent(QCloseEvent *event);
private:
  TelemetryPlot plot;
  TelemetryPlot *pcopy;
  Player player;

  uint flightNo; //loaded flight idx
  uint flightCnt;
  void rescan(void);

  QStringList uavNames,filteredList,filesList;
  QString filter;


  void export_csv(QString fileName);
  void export_fdr(QString fileName);
  void export_kml(QString fileName);
private slots:
  void on_aLast_triggered(void);
  void on_aPrev_triggered(void);
  void on_aNext_triggered(void);
  void on_aReload_triggered(void);
  void on_aDelete_triggered(void);
  void on_lbCurrent_clicked(void);

  void on_aPlay_triggered(void);

  void on_eNotes_returnPressed(void);

  void load(int idx);
  void loadRecent();

  void on_aExport_triggered(void);
  void on_aReport_triggered(void);

  void on_aCut_triggered(void);
  void on_aFiles_triggered(void);
  void on_aEdit_triggered(void);
  void on_aFullScreen_triggered(void);

  void on_aSplitV_triggered(void);

  void on_aFilterUAV_triggered(void);

  void on_avCLR_triggered(void);
  void on_avSTD_triggered(void);
  void on_avIMU_triggered(void);
  void on_avCTR_triggered(void);
};
//=============================================================================
#endif
