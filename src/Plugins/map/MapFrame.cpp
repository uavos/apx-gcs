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
#include <QtWidgets>
#include "MapFrame.h"
#include <math.h>
#include "QMandala.h"
#include "MissionItemWp.h"
#include "AppDirs.h"
//=============================================================================
MapFrame::MapFrame(QWidget *parent)
  :QWidget(parent)
{
  mandala=qApp->property("Mandala").value<QMandala*>();

  setupUi(this);
  QHBoxLayout *layout = new QHBoxLayout(toolBarWidget);
  layout->setMargin(0);
  toolBarWidget->setLayout(layout);

  //fix info bar resize
  /*QToolBar *tbInfo=new QToolBar(this);
  infoLayout->insertWidget(0,tbInfo);
  tbInfo->addWidget(lbN);
  tbInfo->addWidget(lbE);
  tbInfo->addWidget(lbLevel);
  tbInfo->addWidget(lbFps);*/
  //tbInfo->addWidget(lbDownload);




  model=new MissionModel(mapView);

  QActionGroup miActions(this);
  QAction *sep=new QAction(this);
  sep->setSeparator(true);
  miActions.addAction(aAddWaypoint);
  miActions.addAction(aAddRunway);
  miActions.addAction(aAddTaxiway);
  miActions.addAction(aAddPoint);
  miActions.addAction(aAddRestricted);
  miActions.addAction(aAddEmergency);
  miActions.addAction(aAddAreaPoint);
  sep=new QAction(this);
  sep->setSeparator(true);
  miActions.addAction(sep);
  miActions.addAction(aRemove);
  sep=new QAction(this);
  sep->setSeparator(true);
  miActions.addAction(sep);
  miActions.addAction(aWpAdj);
  sep=new QAction(this);
  sep->setSeparator(true);
  miActions.addAction(sep);
  miActions.addAction(aGoWpt);
  miActions.addAction(aGoPi);
  miActions.addAction(aLand);

  QToolBar *toolBar=new QToolBar(toolBarWidget);
  //toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  layout->addWidget(toolBar);
  toolBar->addAction(aGoHome);
  toolBar->addAction(aGoUAV);
  toolBar->addSeparator();
  QToolButton *btn=new QToolButton(toolBar);
  btn->setDefaultAction(aUpload);
  btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  btn->setObjectName("uploadButton");
  toolBar->addWidget(btn);
  toolBar->addSeparator();
  toolBar->addAction(aFullScreen);
  toolBar->addAction(aZoomIn);
  toolBar->addAction(aZoomOut);
  toolBar->addAction(aTraceShow);
  toolBar->addAction(aTraceReset);
  toolBar->addAction(aTraceResetF);
  toolBar->addSeparator();
  toolBar->addAction(aSave);
  toolBar->addAction(aSaveAs);
  toolBar->addAction(aNewFile);
  toolBar->addAction(aLoad);
  toolBar->addSeparator();
  toolBar->addAction(aFromTelemetry);
  toolBar->addAction(aRequest);
  toolBar->addSeparator();
  toolBar->addActions(miActions.actions());
  toolBar->addSeparator();
  toolBar->addAction(aAutoDownload);
  toolBar->addAction(aDownload);
  toolBar->addAction(aStop);

  missionListView=new MissionListView(splitter,model);
  missionListView->tree->addActions(miActions.actions());
  connect(missionListView,SIGNAL(point_clicked(MissionItemObject*)),this,SLOT(listPointClicked(MissionItemObject*)),Qt::QueuedConnection);

  connect(aUpload,SIGNAL(triggered()),model,SLOT(upload()));
  connect(aRequest,SIGNAL(triggered()),model,SLOT(request()));

  connect(aAddWaypoint,SIGNAL(triggered()),model->waypoints,SLOT(add()));
  connect(aAddRunway,SIGNAL(triggered()),model->runways,SLOT(add()));
  connect(aAddTaxiway,SIGNAL(triggered()),model->taxiways,SLOT(add()));
  connect(aAddPoint,SIGNAL(triggered()),model->points,SLOT(add()));
  connect(aAddRestricted,SIGNAL(triggered()),model->restricted,SLOT(add()));
  connect(aAddEmergency,SIGNAL(triggered()),model->emergency,SLOT(add()));
  connect(aAddAreaPoint,SIGNAL(triggered()),model->restricted,SIGNAL(addPointToSelectedObject()));
  connect(aAddAreaPoint,SIGNAL(triggered()),model->emergency,SIGNAL(addPointToSelectedObject()));
  connect(aRemove,SIGNAL(triggered()),model,SLOT(remove()));

  connect(mandala->local->rec,SIGNAL(fileLoaded()),SLOT(modelSelectionChanged()),Qt::QueuedConnection);
  connect(aFromTelemetry,SIGNAL(triggered()),model,SLOT(loadFromTelemetry()));
  aFromTelemetry->setEnabled(false);

  connect(aDownload,SIGNAL(triggered()),&mapView->mapTiles,SLOT(refresh()));
  connect(aAutoDownload,SIGNAL(toggled(bool)),&mapView->mapTiles,SLOT(setAutoDownload(bool)));
  connect(aStop,SIGNAL(triggered()),&(mapView->mapTiles),SLOT(abort()));
  connect(aGoHome,SIGNAL(triggered()),mapView,SLOT(goHome()));
  connect(aGoUAV,SIGNAL(triggered()),mapView,SLOT(goUAV()));

  connect(aDownloadDeep,SIGNAL(toggled(bool)),mapView,SLOT(setShowBlockFrames(bool)));

  connect(mapView,SIGNAL(updateStats()),this,SLOT(updateStats()),Qt::QueuedConnection);
  connect(model->selectionModel,SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,SLOT(modelSelectionChanged()));

  QAction *aSep=new QAction(this);
  aSep->setSeparator(true);

  mapView->addActions(miActions.actions());
  mapView->addAction(aSep);
  mapView->addAction(aSetHome);
  mapView->addAction(aFlyHere);
  mapView->addAction(aLookHere);
  mapView->addAction(aPosFix);
  mapView->addAction(aSep);
  mapView->addAction(aTraceShow);
  mapView->addAction(aTraceReset);
  mapView->addAction(aTraceResetF);

  aUavShowHdg=new QAction(tr("Show heading wheel"),this);
  aUavShowHdg->setCheckable(true);
  //aUavShowHdg->setChecked(QSettings().value("mapShowHdg").toBool());
  connect(aUavShowHdg,SIGNAL(triggered(bool)),this,SLOT(aUavShowHdg_triggered(bool)));
  mapView->addAction(aUavShowHdg);

  connect(aDownloadDeep,SIGNAL(triggered(bool)),this,SLOT(mapToolTriggered(bool)));
  connect(mapView,SIGNAL(mapClicked(MapTile *)),this,SLOT(mapItemClicked(MapTile *)));

  aTraceShow->setChecked(QSettings().value("showTrace",true).toBool());

  //try load last file
  QString fname=QSettings().value("missionFile",AppDirs::missions().absoluteFilePath("xplane-seattle.xml")).toString();
  if(QFile::exists(fname))
    model->loadFromFile(fname);

  connect(mandala,SIGNAL(currentChanged(QMandalaItem*)),this,SLOT(mandalaCurrentChanged(QMandalaItem*)));
  mandalaCurrentChanged(mandala->current);
}
//=============================================================================
bool MapFrame::aboutToQuit()
{
  return saveChanges();
}
//=============================================================================
void MapFrame::mandalaCurrentChanged(QMandalaItem *m)
{
  Q_UNUSED(m)
  aUavShowHdg->setChecked(false);
}
//=============================================================================
void MapFrame::updateStats()
{
  int dlcnt=mapView->mapTiles.download_cnt();
  lbDownload->setVisible(dlcnt);
  if(dlcnt)lbDownload->setText(QString().sprintf("%s..%u",tr("Downloading").toUtf8().data(),dlcnt));

  //cursor pos
  double lat=mapView->curLL.x(),lon=mapView->curLL.y();
  lbN->setText(QMandala::latToString(lat));
  lbN->setToolTip(QString().sprintf("%s: %f",tr("Lat").toUtf8().data(),lat));
  lbE->setText(QMandala::lonToString(lon));
  lbE->setToolTip(QString().sprintf("%s: %f",tr("Lon").toUtf8().data(),lon));

  lbLevel->setText(QString().sprintf("%s:%u",tr("L").toUtf8().data(),mapView->mapTiles.level));

  lbFps->setText(QString().sprintf("%s:%u",tr("FPS").toUtf8().data(),mapView->fps));

  //calc distance to home
  double distHome=mapView->curLL.isNull()?0:mandala->current->distance(mandala->current->llh2ne(Vect(lat,lon,mandala->current->home_pos[2])));
  double distMouse=mapView->clkLL.isNull()?0:mandala->current->distance(mandala->current->llh2ne(Vect(mapView->clkLL.x(),mapView->clkLL.y(),mandala->current->home_pos[2]),Vect(mapView->curLL.x(),mapView->curLL.y(),mandala->current->home_pos[2])));
  lbDM->setText(QString().sprintf("<font color='#484'>%s</font>:%.2f <font color=gray>km</font>",tr("DM").toUtf8().data(),distMouse/1000.0));
  lbDH->setText(QString().sprintf("<font color='#4f4'>%s</font>:%.2f <font color=gray>km</font>",tr("DH").toUtf8().data(),distHome/1000.0));
  lbPD->setText(QString().sprintf("<font color=cyan>%s:</font>%.0f <font color=gray>km</font>",tr("PD").toUtf8().data(),model->waypoints->childCount()?static_cast<MissionItemWp*>(model->waypoints->childItems.last())->DME()/1000.0:0));
  if(mapView->currentUAV){
    lbFD->setText(QString().sprintf("<font color='#88f'>%s:</font>%.1f <font color=gray>km</font>",tr("FD").toUtf8().data(),mapView->currentUAV->dist_file/1000.0));
    lbTD->setText(QString().sprintf("<font color='#f44'>%s:</font>%.1f <font color=gray>km</font>",tr("TD").toUtf8().data(),mapView->currentUAV->dist/1000.0));
  }

  aSave->setEnabled(model->isModified());
}
//=============================================================================
void MapFrame::modelSelectionChanged()
{
  uint wp=model->waypoints->selectedObjects().size();
  uint rw=model->runways->selectedObjects().size();
  uint tw=model->taxiways->selectedObjects().size();
  uint pi=model->points->selectedObjects().size();
  uint rst=model->restricted->selectedObjects().size()>0;
  uint emg=model->emergency->selectedObjects().size()>0;
  aRemove->setEnabled(model->selectedItems().size()>0);
  aAddAreaPoint->setEnabled(rst^emg);
  aWpAdj->setEnabled(wp&&(!rw)&&(!tw)&&(!pi));
  aLand->setEnabled(rw==1);
  aGoWpt->setEnabled(wp==1);
  aGoPi->setEnabled(pi==1);
  aFromTelemetry->setEnabled(mandala->local->rec->file.xmlParts.contains("mission"));
}
//=============================================================================
void MapFrame::on_aFullScreen_triggered(void)
{
  if(windowState()==Qt::WindowFullScreen){
    showNormal();
    setWindowFlags(wf_save);
    showNormal();
    if(parent()->inherits("QDockWidget")){
      //static_cast<QDockWidget*>(parent())->setFloating(df_save);
      static_cast<QMainWindow*>(static_cast<QDockWidget*>(parent())->parent())->restoreState(geometry_save);
    }
    //restoreGeometry(geometry_save);
  }else {
    wf_save=windowFlags();
    if(parent()->inherits("QDockWidget")){
      df_save=static_cast<QDockWidget*>(parent())->isFloating();
      geometry_save=static_cast<QMainWindow*>(static_cast<QDockWidget*>(parent())->parent())->saveState();
      static_cast<QDockWidget*>(parent())->setFloating(true);
    }
    setWindowFlags(Qt::Window);
    showFullScreen();
  }
}
//=============================================================================
void MapFrame::mapToolTriggered(bool checked)
{
  if (checked) {
    mapView->setDragMode(QGraphicsView::RubberBandDrag);
  } else {
    mapView->setDragMode(QGraphicsView::ScrollHandDrag);
  }
}
//=============================================================================
void MapFrame::mapItemClicked(MapTile *item)
{
  mapToolTriggered(false);
  if (aDownloadDeep->isChecked()) {
    aDownloadDeep->setChecked(false);
    mapView->mapTiles.downloadDeep(item->tile_name);
  }
}
//=============================================================================
void MapFrame::listPointClicked(MissionItemObject *item)
{
  mapView->goItem(item->mapItem);
}
//=============================================================================
void MapFrame::on_aZoomIn_triggered()
{
  mapView->zoomIn();
}
void MapFrame::on_aZoomOut_triggered()
{
  mapView->zoomOut();
}
//=============================================================================
//=============================================================================
void MapFrame::on_aTraceReset_triggered()
{
  mapView->currentUAV->clearTrace();
}
void MapFrame::on_aTraceResetF_triggered()
{
  mapView->currentUAV->clearTraceF();
}
void MapFrame::on_aTraceShow_toggled(bool checked)
{
  QSettings().setValue("showTrace",checked);
  foreach(ItemUav *i,mapView->iUAV)
    i->setShowTrace(checked);
}
void MapFrame::aUavShowHdg_triggered(bool checked)
{
  mapView->currentUAV->setShowHdg(checked);
}
//=============================================================================
void MapFrame::on_aSetHome_triggered()
{
  if(mapView->clkLL.isNull()) return;
  mandala->current->home_pos[0]=mapView->clkLL.x();
  mandala->current->home_pos[1]=mapView->clkLL.y();
  mandala->current->send(idx_home_pos);
}
//=============================================================================
void MapFrame::on_aFlyHere_triggered()
{
  if(mapView->clkLL.isNull()) return;
  mandala->current->cmd_NE=mandala->current->lla2ne(Vect(mapView->clkLL.x(),mapView->clkLL.y(),mandala->current->cmd_altitude));
  mandala->current->send(idx_cmd_NE);
}
//=============================================================================
void MapFrame::on_aLookHere_triggered()
{
  if(mapView->clkLL.isNull()) return;
  mandala->current->cam_tpos=Vect(mapView->clkLL.x(),mapView->clkLL.y(),mandala->current->home_pos[2]);
  mandala->current->send(idx_cam_tpos);
}
//=============================================================================
void MapFrame::on_aPosFix_triggered()
{
  if(mapView->clkLL.isNull()) return;
  mandala->current->gps_pos=Vect(mapView->clkLL.x(),mapView->clkLL.y(),mandala->current->altitude+mandala->current->home_pos[2]);
  mandala->current->send(idx_gps_pos);
}
//=============================================================================
void MapFrame::on_aWpAdj_triggered()
{
  const QList<MissionItemObject*> &list=model->waypoints->selectedObjects();
  if(!list.size())return;
  bool ok;
  int v=QInputDialog::getInt(this,static_cast<QAction*>(sender())->text(),tr("Change %1 waypoint(s):").arg(list.size()),0,-10000,10000,10,&ok);
  if(!ok || v==0)return;
  foreach(MissionItem *i,list)
    static_cast<MissionItemWp*>(i)->f_altitude->setValue(static_cast<MissionItemWp*>(i)->f_altitude->value().toInt()+v);
}
//=============================================================================
void MapFrame::on_aLand_triggered()
{
  const QList<MissionItemObject*> &list=model->runways->selectedObjects();
  if(list.size()!=1)return;
  mandala->current->rwidx=list.first()->row();
  mandala->current->send(idx_rwidx);
}
void MapFrame::on_aGoWpt_triggered()
{
  const QList<MissionItemObject*> &list=model->waypoints->selectedObjects();
  if(list.size()!=1)return;
  mandala->current->wpidx=list.first()->row();
  mandala->current->send(idx_wpidx);
}
void MapFrame::on_aGoPi_triggered()
{
  const QList<MissionItemObject*> &list=model->points->selectedObjects();
  if(list.size()!=1)return;
  mandala->current->piidx=list.first()->row();
  mandala->current->send(idx_piidx);
}
//=============================================================================
//=============================================================================
bool MapFrame::saveChanges(void)
{
  if(!model->isModified())return true;
  int rv=QMessageBox::question(
    this,QApplication::applicationName(),
    tr("Mission '%1' was modified. Save changes?").arg(model->missionName),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
  if(rv==QMessageBox::Cancel)return false;
  if(rv==QMessageBox::Yes){
    aSave->trigger();
    if(model->isModified())return false;
  }
  return true;
}
//=============================================================================
void MapFrame::on_aSave_triggered()
{
  if(model->missionFileName.isEmpty()){
    aSaveAs->trigger();
    return;
  }
  model->saveToFile(model->missionFileName);
}
//=============================================================================
void MapFrame::on_aSaveAs_triggered()
{
  if(model->isEmpty())return;

  QFileDialog dlg(this,aSave->toolTip(),AppDirs::missions().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  dlg.selectFile(AppDirs::missions().filePath(model->missionName+".xml"));
  QStringList filters;
  filters << tr("XML Files")+" (*.xml)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  if(!dlg.exec() || !dlg.selectedFiles().size())return;
  model->saveToFile(dlg.selectedFiles().at(0));
  QSettings().setValue("missionFile",dlg.selectedFiles().at(0));
}
//=============================================================================
void MapFrame::on_aNewFile_triggered()
{
  if(!saveChanges())return;
  model->newFile();
}
//=============================================================================
void MapFrame::on_aLoad_triggered()
{
  if(!saveChanges())return;
  QString fname =QFileDialog::getOpenFileName(this,aSave->toolTip(),QSettings().value("wptFileDir",AppDirs::missions().canonicalPath()).toString(),tr("XML Files")+" (*.xml)");
  if (!QFile::exists(fname))return;
  model->loadFromFile(fname);
  QSettings().setValue("missionFileDir",QFileInfo(fname).absolutePath());
  QSettings().setValue("missionFile",fname);
}
//=============================================================================
