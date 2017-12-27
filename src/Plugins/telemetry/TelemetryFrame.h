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
#include "TelemetryPlot.h"
#include "TelemetryPlayer.h"
#include <QtCore>
#include <TelemetryDB.h>
//=============================================================================
class TelemetryFrame: public QWidget
{
  Q_OBJECT
public:
  explicit TelemetryFrame(QWidget *parent = 0);

protected:
  void closeEvent(QCloseEvent *event);
private:
  TelemetryDB *_db;


  QToolBar *toolBar;
  QVBoxLayout *vlayout;

  QAction *aLast;
  QAction *aReload;
  QAction *aPrev;
  QAction *aNext;

  QAction *aFilter;
  QAction *aFullScreen;
  QAction *aSplit;

  QAction *aExport;
  QAction *aImport;
  QAction *aRestore;
  QAction *aDelete;

  QAction *aReplay;

  TelemetryPlot *plot;
  TelemetryPlot *pcopy;
  QLabel *lbTitle;

  QLineEdit *eNotes;
  QProgressBar *progressBar;

  QToolBar *toolBarSW;
  QAction *avCLR;
  QAction *avSTD;
  QAction *avIMU;
  QAction *avCTR;

  QToolBar *toolBarPlayer;
  TelemetryPlayer *player;
  QSlider *playerSlider;
  QDoubleSpinBox *playerSpeed;
  QLabel *lbPlayerTime;
  QAction *aPlay;
  QAction *aPause;
  QAction *aRewind;



  quint64 curID; //loaded flight
  quint64 curTimestamp; //loaded flight
  quint64 recCnt;
  quint64 recNum;
  QString recNotes;
  quint64 recTimeMax;
  quint64 recSize;

  bool bLoading;
  int m_progress;

  void resetPlot();
  void rescan(void);
  void load(QSqlQuery &query,bool forceLarge=false);

  bool getPrev(QSqlQuery &query);

  QStringList uavNames,filteredList,filesList;
  QString filter;

  QDockWidget *parentW;

  void export_csv(QString fileName);
  void export_fdr(QString fileName);
  void export_kml(QString fileName);
private slots:
  void aLast_triggered(void);
  void aPrev_triggered(void);
  void aNext_triggered(void);
  void aReload_triggered(void);
  void aFilter_triggered(void);
  void eNotes_returnPressed(void);

  void aExport_triggered(void);
  void aImport_triggered(void);
  void aRestore_triggered(void);
  void aDelete_triggered(void);

  void aReplay_triggered(void);

  void aFullScreen_triggered(void);
  void aSplit_triggered(void);


  void avCLR_triggered(void);
  void avSTD_triggered(void);
  void avIMU_triggered(void);
  void avCTR_triggered(void);

  void playerSliderMoved();
  void plotTimeCursorMoved();
  void playerTimeChanged();

  void setProgress(int v);
};
//=============================================================================
#endif
