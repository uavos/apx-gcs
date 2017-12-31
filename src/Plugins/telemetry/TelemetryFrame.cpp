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
#include "TelemetryFrame.h"
#include <QtGui>
#include <QtNetwork>
#include <QDomDocument>
#include <Facts.h>
#include <AppDirs.h>
#include <SvgIcon.h>
#include "TelemetryXml.h"
//=============================================================================
TelemetryFrame::TelemetryFrame(QWidget *parent)
  : QWidget(parent),
    player(NULL),
    curID(0), recCnt(0),recNum(0),
    recTimeMax(0),recSize(0),
    bLoading(false), m_progress(0)
{
  setWindowTitle(tr("Telemetry"));
  //setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);

  //database
  _db = new TelemetryDB(this,QLatin1String("GCSTelemetryPluginSession"));

  if(!QSettings().contains("Qwt_AntiAliased"))QSettings().setValue("Qwt_AntiAliased",false);

  vlayout=new QVBoxLayout(this);
  setLayout(vlayout);
  vlayout->setMargin(0);
  vlayout->setSpacing(0);

  toolBar=new QToolBar(this);
  toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  vlayout->addWidget(toolBar);

  toolBar=new QToolBar(this);
  toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  vlayout->addWidget(toolBar);

  toolBarPlayer=new QToolBar(this);
  toolBarPlayer->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBarPlayer->setVisible(false);
  vlayout->addWidget(toolBarPlayer);

  plot=new TelemetryPlot(this);
  plot->resetZoom();
  vlayout->addWidget(plot);
  //connect(plot,&TelemetryPlot::progressChanged,this,&TelemetryFrame::setProgress);

  lbTitle=new QLabel(this);
  lbTitle->setParent(plot);
  lbTitle->setStyleSheet("background-color: rgba(0,0,0,30%)");
  lbTitle->setAttribute(Qt::WA_TransparentForMouseEvents);

  progressBar=new QProgressBar(this);
  progressBar->setStyleSheet("QProgressBar {height: 10px; text-align: center; border: 1px solid gray;padding: 0px;background: black;} QProgressBar::chunk {background: gray;border: 0px;}");
  progressBar->setMaximum(100);
  progressBar->setVisible(false);
  progressBar->setParent(plot);
  progressBar->adjustSize();
  progressBar->resize(400,progressBar->height());
  progressBar->move(2,2);
  //progressBar->move(2,lbTitle->height()+2);


  //actions
  aLast=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-fastforward.svg"),tr("Load recent"),this,&TelemetryFrame::aLast_triggered);
  aReload=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-refresh.svg"),tr("Reload"),this,&TelemetryFrame::aReload_triggered);
  toolBar->addSeparator();
  aPrev=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-arrow-back.svg"),tr("Load previous"),this,&TelemetryFrame::aPrev_triggered);
  aNext=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-arrow-forward.svg"),tr("Load next"),this,&TelemetryFrame::aNext_triggered);
  toolBar->addSeparator();
  aFilter=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/eye.svg"),tr("Filter vehicle"),this,&TelemetryFrame::aFilter_triggered);
  aFilter->setCheckable(true);
  toolBar->addSeparator();
  aFullScreen=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-expand.svg"),tr("Full screen"),this,&TelemetryFrame::aFullScreen_triggered);
  aFullScreen->setCheckable(true);
  //aSplit=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-book-outline.svg"),tr("Split view"),this,&TelemetryFrame::aSplit_triggered);
  //aSplit->setCheckable(true);
  toolBar->addSeparator();

  aExport=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/share.svg"),tr("Export"),this,&TelemetryFrame::aExport_triggered);
  aImport=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/android-folder-open.svg"),tr("Import"),this,&TelemetryFrame::aImport_triggered);
  aRestore=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-undo.svg"),tr("Restore"),this,&TelemetryFrame::aRestore_triggered);
  toolBar->addSeparator();
  aDelete=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/ios-trash-outline.svg"),tr("Delete"),this,&TelemetryFrame::aDelete_triggered);
  toolBar->addSeparator();

  aReplay=toolBar->addAction(SvgIcon(":/icons/sets/ionicons/play.svg"),tr("Replay"),this,&TelemetryFrame::aReplay_triggered);
  aReplay->setCheckable(true);
  toolBar->addSeparator();

  eNotes=new QLineEdit(this);
  eNotes->setPlaceholderText(tr("Notes"));
  connect(eNotes,&QLineEdit::returnPressed,this,&TelemetryFrame::eNotes_returnPressed);
  toolBar->addWidget(eNotes);

  toolBarSW=new QToolBar(this);
  toolBarSW->setObjectName("toolBarSW");
  toolBarSW->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBarSW->layout()->setMargin(0);
  toolBar->addWidget(toolBarSW);
  avCLR=toolBarSW->addAction(tr("CLR"),this,&TelemetryFrame::avCLR_triggered);
  avSTD=toolBarSW->addAction(tr("STD"),this,&TelemetryFrame::avSTD_triggered);
  avIMU=toolBarSW->addAction(tr("IMU"),this,&TelemetryFrame::avIMU_triggered);
  avCTR=toolBarSW->addAction(tr("CTR"),this,&TelemetryFrame::avCTR_triggered);

  //player
  aPlay=toolBarPlayer->addAction(SvgIcon(":/icons/sets/ionicons/play.svg"),tr("Play"));
  aPause=toolBarPlayer->addAction(SvgIcon(":/icons/sets/ionicons/pause.svg"),tr("Pause"));
  aRewind=toolBarPlayer->addAction(SvgIcon(":/icons/sets/ionicons/ios-rewind.svg"),tr("Rewind"));
  playerSpeed=new QDoubleSpinBox(this);
  toolBarPlayer->addWidget(playerSpeed);
  playerSpeed->setMinimum(0.100);
  playerSpeed->setMaximum(100);
  playerSpeed->setSingleStep(0.1);
  playerSpeed->setAccelerated(true);
  playerSpeed->setDecimals(3);
  playerSlider=new QSlider(Qt::Horizontal,this);
  toolBarPlayer->addWidget(playerSlider);
  connect(playerSlider,&QSlider::sliderMoved,this,&TelemetryFrame::playerSliderMoved);
  connect(plot,&TelemetryPlot::timeCursorChanged,this,&TelemetryFrame::plotTimeCursorMoved);
  lbPlayerTime=new QLabel(this);
  toolBarPlayer->addWidget(lbPlayerTime);
  //lbPlayerTime->setMinimumWidth(lbPlayerTime->font().pointSize()*8);
  //lbPlayerTime->setStyleSheet("background-color: rgba(0,0,0,30%)");
  lbPlayerTime->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

  //create plot fields
  foreach(VehicleMandalaFact *f,Vehicles::instance()->f_local->f_mandala->allFacts){
    //fill params
    uint type=f->_vtype;
    uint varmsk=f->id();
    QString sn=f->name();
    uint ci=0;
    if(type==vt_vect || type==vt_point) ci=(varmsk>>8)+1;
    QColor c(Qt::cyan);
    if(sn.contains("ctr_"))c=Qt::magenta;
    else if(sn.contains("ctrb_"))c=Qt::magenta;
    else if(type==vt_flag)c=QColor(Qt::blue).lighter();
    else if(ci==1)c=Qt::red;
    else if(ci==2)c=Qt::green;
    else if(ci==3)c=Qt::yellow;
    //if(sv.size()>=2)c=QColor(sv.at(1));
    //if(sv.size()>=3)divider_dsp=sv.at(2).toDouble();
    Qt::PenStyle style = Qt::SolidLine;
    if(sn.contains("cmd_"))style=Qt::DotLine;
    else if(sn.contains("gps_"))style=Qt::DashLine;
    else if(sn.contains("rc_"))style=Qt::DotLine;
    TelemetryFieldData *d=new TelemetryFieldData;
    d->fact=f;
    telemetryData.fields.append(d);
    QwtPlotCurve *cv=plot->addCurve(sn,f->descr(),f->units(),QPen(c, 0, style));
    plotMap.insert(d,cv);
  }

  //empty plot after start
  rescan();
  lbTitle->setText(QString("%1").arg(recCnt));
  lbTitle->adjustSize();
}
//=============================================================================
void TelemetryFrame::setProgress(int v)
{
  if(m_progress==v)return;
  if(v<=0){
    m_progress=v;
    progressBar->setValue(0);
    progressBar->setVisible(false);
    lbTitle->setVisible(true);
    setEnabled(true);
    return;
  }
  if(m_progress==0){
    setEnabled(false);
    progressBar->setVisible(true);
    lbTitle->setVisible(false);
  }
  m_progress=v;
  progressBar->setValue(v);
  for(int i=0;i<100;i++) QCoreApplication::processEvents();
}
//=============================================================================
void TelemetryFrame::aPrev_triggered(void)
{
  if(!curID){
    aLast->trigger();
    return;
  }
  if(!_db)return;
  QSqlQuery query(*_db);
  if(getPrev(query))load(query);
}
bool TelemetryFrame::getPrev(QSqlQuery &query)
{
  bool ok=true;
  while(ok){
    if(filter.isEmpty()) query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND timestamp <= ? ORDER BY timestamp DESC, key DESC LIMIT 100");
    else{
      query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND callsign=? AND timestamp <= ? ORDER BY timestamp DESC, key DESC LIMIT 100");
      query.addBindValue(filter);
    }
    query.addBindValue(curTimestamp);
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      if(query.value(0).toULongLong()!=curID)continue;
      if(!query.next())break;
      return true;
    }
    break;
  }
  return false;
}
void TelemetryFrame::aNext_triggered(void)
{
  if(!curID){
    aLast->trigger();
    return;
  }
  if(!_db)return;
  QSqlQuery query(*_db);
  bool ok=true;
  while(ok){
    if(filter.isEmpty()) query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND timestamp >= ? ORDER BY timestamp ASC, key ASC LIMIT 100");
    else{
      query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND callsign=? AND timestamp >= ? ORDER BY timestamp ASC, key ASC LIMIT 100");
      query.addBindValue(filter);
    }
    query.addBindValue(curTimestamp);
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      if(query.value(0).toULongLong()!=curID)continue;
      if(!query.next())break;
      load(query);
    }
    break;
  }
}
void TelemetryFrame::aLast_triggered(void)
{
  rescan();
  if(!_db)return;
  QSqlQuery query(*_db);
  bool ok=true;
  while(ok){
    //find the latest telemetryID
    if(filter.isEmpty()) query.prepare("SELECT * FROM Telemetry WHERE rec=1 ORDER BY timestamp DESC, key DESC LIMIT 1");
    else{
      query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND callsign=? ORDER BY timestamp DESC, key DESC LIMIT 1");
      query.addBindValue(filter);
    }
    ok=query.exec() && query.next();
    if(!ok)break;
    load(query);
    break;
  }
  if(ok)return;
  //clear plot
  lbTitle->setText("0");
  lbTitle->adjustSize();
  resetPlot();
}
void TelemetryFrame::aReload_triggered(void)
{
  if(!curID){
    aLast->trigger();
    return;
  }
  if(!_db)return;
  QSqlQuery query(*_db);
  bool ok=true;
  while(ok){
    query.prepare("SELECT * FROM Telemetry WHERE key = ?");
    query.addBindValue(curID);
    ok=query.exec() && query.next();
    if(!ok)break;
    load(query,true);
    break;
  }
}
//=============================================================================
void TelemetryFrame::rescan(void)
{
  recCnt=0;
  uavNames.clear();

  if(!_db)return;
  QSqlQuery query(*_db);
  bool ok=true;
  while(ok){
    //find number of records
    if(filter.isEmpty()) query.prepare("SELECT COUNT(*) FROM Telemetry WHERE rec=1");
    else{
      query.prepare("SELECT COUNT(*) FROM Telemetry WHERE rec=1 AND callsign=?");
      query.addBindValue(filter);
    }
    ok=query.exec() && query.next();
    if(!ok)break;
    recCnt=query.value(0).toULongLong();
    //find vehicle names
    query.prepare("SELECT DISTINCT callsign FROM Telemetry WHERE rec=1 ORDER BY callsign");
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      uavNames.append(query.value(0).toString());
    }
    break;
  }
}
//=============================================================================
void TelemetryFrame::load(QSqlQuery &query, bool forceLarge)
{
  if(bLoading)return;
  bLoading=true;
  curID=query.value(0).toULongLong();
  curTimestamp=query.value("timestamp").toULongLong();
  QString callsign=query.value("callsign").toString();
  QString comment=query.value("comment").toString();

  recNotes=query.value("notes").toString();
  query.finish();

  QString recStats,recStatus;
  recTimeMax=0;
  recSize=0;

  //clear plot
  resetPlot();

  bool ok=true;
  while(ok){
    //find current number
    if(filter.isEmpty()) query.prepare("SELECT COUNT(*) FROM Telemetry WHERE rec=1 AND timestamp <= ?");
    else{
      query.prepare("SELECT COUNT(*) FROM Telemetry WHERE rec=1 AND callsign=? AND timestamp <= ?");
      query.addBindValue(filter);
    }
    query.addBindValue(curTimestamp);
    ok=query.exec() && query.next();
    if(!ok)break;
    recNum=query.value(0).toULongLong();
    if(filter.isEmpty()) query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND timestamp <= ? ORDER BY timestamp DESC, key DESC LIMIT 100");
    else{
      query.prepare("SELECT * FROM Telemetry WHERE rec=1 AND callsign=? AND timestamp <= ? ORDER BY timestamp DESC, key DESC LIMIT 100");
      query.addBindValue(filter);
    }
    query.addBindValue(curTimestamp);
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      if(query.value(0).toULongLong()==curID)break;
      recNum--;
    }
    query.finish();

    //collect fields map
    QHash<quint64,TelemetryFieldData*> fmap;
    query.prepare("SELECT key, name FROM TelemetryFields");
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      QString s=query.value(1).toString();
      for(int i=0;i<telemetryData.fields.size();i++){
        if(telemetryData.fields.at(i)->fact->name()!=s)continue;
        fmap.insert(query.value(0).toULongLong(),telemetryData.fields.at(i));
        break;
      }
    }
    //find number of records
    query.prepare("SELECT COUNT(*) FROM TelemetryDownlink WHERE TelemetryID=?");
    query.addBindValue(curID);
    ok=query.exec() && query.next();
    if(!ok)break;
    recSize=query.value(0).toULongLong();
    query.finish();
    if(recSize>800000){
      if(forceLarge==false){
        recStatus=QString("** %1 **").arg(tr("large set - hit 'Reload' to show data"));
        //qWarning("%s",recStatus.toUtf8().data());
        break;
      }
    }
    //find statistics
    QStringList stStats;
    query.prepare("SELECT COUNT(*) FROM TelemetryUplink WHERE TelemetryID=?");
    query.addBindValue(curID);
    ok=query.exec() && query.next();
    if(!ok)break;
    quint64 recSizeUplink=query.value(0).toULongLong();
    if(recSizeUplink)stStats.append(QString("%1: %2").arg("uplink").arg(recSizeUplink));
    query.prepare("SELECT COUNT(*), name FROM TelemetryEvents WHERE telemetryID=? GROUP BY name ORDER BY name");
    query.addBindValue(curID);
    ok=query.exec();
    if(!ok)break;
    while(query.next()){
      quint64 ecnt=query.value(0).toULongLong();
      if(ecnt)stStats.append(QString("%1: %2").arg(query.value(1).toString()).arg(ecnt));
    }
    query.finish();
    if(!stStats.isEmpty())recStats=stStats.join(" | ");
    //collect data
    quint64 t0=0;
    telemetryData.times.append(0);
    const quint64 rowLimit=100000;
    for(quint64 row=0;row<recSize;row+=rowLimit){
      setProgress(row*100/recSize);
      query.prepare("SELECT fieldID, time, value FROM TelemetryDownlink WHERE TelemetryID=? LIMIT ? OFFSET ?");
      query.addBindValue(curID);
      query.addBindValue(rowLimit);
      query.addBindValue(row);
      ok=query.exec();
      if(!ok)break;
      while(query.next()){
        TelemetryFieldData *f=fmap.value(query.value(0).toULongLong());
        if(!f)continue;
        quint64 t=query.value(1).toULongLong();
        if(!t0)t0=t;
        t-=t0;
        if(recTimeMax<t)recTimeMax=t;
        double tf=t/1000.0;
        if(telemetryData.times.last()!=tf)telemetryData.times.append(tf);
        if(f->points.size() && (tf-f->points.last().x())>0.5){
          //extrapolate unchanged value tail-1ms
          f->points.append(QPointF(tf-0.001,f->points.last().y()));
        }
        f->points.append(QPointF(tf,query.value(2).toDouble()));
      }
      query.finish();
    }//for rows
    if(!ok)break;
    //final data tail at max time
    double tMax=recTimeMax/1000.0;
    for(int i=0;i<telemetryData.fields.size();i++){
      TelemetryFieldData *d=telemetryData.fields.at(i);
      if(d->points.isEmpty())continue;
      if(d->points.last().x()>=tMax)continue;
      d->points.append(QPointF(tMax,d->points.last().y()));
    }
    //loaded
    break;
  }//while ok
  query.finish();

  //update title
  QString s=QDateTime::fromMSecsSinceEpoch(curTimestamp).toString("yyyy MMM dd hh:mm:ss");
  s.append(QString(" (%1)").arg(FactSystem::timeToString(recTimeMax/1000,true)));
  if(!callsign.isEmpty())s.append("\t").append(callsign);
  if(!comment.isEmpty())s.append(" | ").append(comment);
  if(recSize>0){
    QString srcnt=recSize>1000000?QString("%1M").arg(recSize/1000000):recSize>1000?QString("%1K").arg(recSize/1000):QString("%1").arg(recSize);
    s.append(QString("\t(%1 %2)").arg(srcnt).arg(tr("records")));
  }

  lbTitle->setText(QString("%1/%2\t%3%4%5").arg(recNum).arg(recCnt).arg(s).arg(recStats.isEmpty()?recStats:recStats.prepend("\n\t")).arg(recStatus.isEmpty()?recStatus:recStatus.prepend("\n\t").toUpper()));
  lbTitle->adjustSize();
  //aExport->setEnabled(ok);
  //aDelete->setEnabled(ok);
  //aNext->setEnabled((flightNo+1)<flightCnt);
  //aPrev->setEnabled(flightNo>0);
  setEnabled(true);
  eNotes->setText(recNotes);

  //load data to plot
  for(int i=0;i<telemetryData.fields.size();i++){
    TelemetryFieldData *d=telemetryData.fields.at(i);
    QwtPlotCurve *c=plotMap.value(d);
    if(!c)continue;
    c->setSamples(d->points);
  }
  plot->resetZoom();

  bLoading=false;
  setProgress(0);
}
//=============================================================================
//=============================================================================
void TelemetryFrame::resetPlot()
{
  telemetryData.times.clear();
  for(int i=0;i<telemetryData.fields.size();i++){
    TelemetryFieldData *d=telemetryData.fields.at(i);
    d->points.clear();
  }
  foreach (QwtPlotCurve *c, plotMap.values()) {
    c->setSamples(QVector<QPointF>());
  }
  //plot->showCurve(plot->itemToInfo(plot->fcalculated->curve),false);
  plot->resetZoom();
  if(aReplay->isChecked())aReplay->trigger();
  plot->setTimeCursor(0);
}
//=============================================================================
void TelemetryFrame::aFilter_triggered(void)
{
  if(aFilter->isChecked()){
    aFilter->setChecked(!filter.isEmpty());
    rescan();
    bool ok;
    QString item=QInputDialog::getItem(NULL,aFilter->toolTip(),aFilter->text(),uavNames,0,false,&ok);
    if(!ok) return;
    filter=item;
  }else filter="";
  aFilter->setChecked(!filter.isEmpty());
  aLast->trigger();
}
//=============================================================================
void TelemetryFrame::eNotes_returnPressed(void)
{
  QString s=eNotes->text().trimmed();
  if(curID==0 || recNotes==s)return;
  if(!_db->writeNotes(curID,s))return;
  recNotes=s;
  plot->setFocus();
}
//=============================================================================
void TelemetryFrame::aSplit_triggered(void)
{
  /*if(!((QAction*)sender())->isChecked()){
    vlayout->removeWidget(pcopy);
    if(pcopy)pcopy->deleteLater();
    pcopy=NULL;
    return;
  }
  pcopy=new TelemetryPlot();
  pcopy->copyFromPlot(plot);
  vlayout->addWidget(pcopy);*/
}
//=============================================================================
void TelemetryFrame::aFullScreen_triggered(void)
{
  if(windowState()==Qt::WindowFullScreen){
    setWindowFlags(Qt::Widget);
    showNormal();
    if(parentW) parentW->setWidget(this);
  }else{
    parentW=qobject_cast<QDockWidget*>(parentWidget());
    setParent(NULL);
    setWindowFlags(Qt::Window);
    showFullScreen();
  }
}
//=============================================================================
void TelemetryFrame::closeEvent(QCloseEvent *event)
{
  if(windowState()==Qt::WindowFullScreen){
    event->ignore();
    aFullScreen->trigger();
  }
}
//=============================================================================
void TelemetryFrame::avCLR_triggered(void)
{
  plot->showCurves(false);
}
void TelemetryFrame::avSTD_triggered(void)
{
  QStringList st;
  st<<"roll"<<"cmd_roll"<<"pitch"<<"cmd_pitch";
  st<<"altitude"<<"cmd_altitude";
  st<<"airspeed"<<"cmd_airspeed";
  st<<"vspeed";
  plot->showCurves(true,st,true);
}
void TelemetryFrame::avIMU_triggered(void)
{
  QStringList st;
  st<<"Ax"<<"Ay"<<"Az";
  st<<"p"<<"q"<<"r";
  st<<"Hx"<<"Hy"<<"Hz";
  plot->showCurves(true,st,true);
}
void TelemetryFrame::avCTR_triggered(void)
{
  QStringList st;
  foreach (QString vn,Vehicles::instance()->f_local->f_mandala->names)
    if(vn.startsWith("ctr"))st.append(vn);
  plot->showCurves(true,st,true);
}
//=============================================================================
//=============================================================================
void TelemetryFrame::aDelete_triggered(void)
{
  if(!curID)return;
  if(!_db->isOpen())return;
  QSqlQuery query(*_db);
  bool bPrev=getPrev(query);
  _db->deleteRecord(curID);
  if(bPrev){
    rescan();
    load(query);
  }else aLast->trigger();
}
//=============================================================================
void TelemetryFrame::aRestore_triggered(void)
{
}
//=============================================================================
void TelemetryFrame::aExport_triggered(void)
{
  /*if (!Vehicles::instance()->f_local->f_recorder->file.time.size())return;
  QString fileName(Vehicles::instance()->f_local->f_recorder->loadFile.fileName());

  QFileDialog dlg(this,aExport->toolTip(),QSettings().value("saveXplaneFileDir").toString());
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite,false);
  dlg.selectFile(QFileInfo(fileName).baseName());
  QStringList filters;
  filters << tr("Comma separated values")+" (*.csv)"
          << tr("X-plane flight data")+" (*.fdr)"
          << tr("Google Earth KML")+" (*.kml)";
  dlg.setNameFilters(filters);
  dlg.setViewMode(QFileDialog::Detail);
  dlg.setOption(QFileDialog::DontUseNativeDialog,true);

  if(!dlg.exec() || !dlg.selectedFiles().size())return;
  fileName=dlg.selectedFiles().at(0);
  switch(filters.indexOf(dlg.selectedNameFilter())){
    case 0:export_csv(fileName+".csv"); break;
    case 1:export_fdr(fileName+".fdr"); break;
    case 2:export_kml(fileName+".kml"); break;
    default: qDebug("%s: %s",tr("Unknown filter selected").toUtf8().data(),dlg.selectedNameFilter().toUtf8().data());
  }
  qDebug("#%s.",tr("File exported").toUtf8().data());*/
}
//=============================================================================
void TelemetryFrame::aImport_triggered(void)
{
  QFileDialog dlg(this,aImport->text(),AppDirs::telemetry().canonicalPath());
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  dlg.setFileMode(QFileDialog::ExistingFile);
  QStringList filters;
  filters << tr("Telemetry files")+" (*.datalink)"
          << tr("Any files")+" (*)";
  dlg.setNameFilters(filters);
  if(!dlg.exec() || dlg.selectedFiles().isEmpty())return;

  TelemetryXml xml;
  connect(&xml,&TelemetryXml::progressChanged,this,&TelemetryFrame::setProgress);

  quint64 telemetryID=xml.read(dlg.selectedFiles().first());
  if(!telemetryID)return;

  rescan();
  if(!_db)return;
  QSqlQuery query(*_db);
  bool ok=true;
  while(ok){
    query.prepare("SELECT * FROM Telemetry WHERE key = ?");
    query.addBindValue(telemetryID);
    ok=query.exec() && query.next();
    if(!ok)break;
    load(query);
    break;
  }
}
//=============================================================================
//=============================================================================
//=============================================================================
void TelemetryFrame::aReplay_triggered(void)
{
  bool bShow=aReplay->isChecked();
  if(bShow && (!recTimeMax)){
    aReplay->trigger();
    return;
  }
  toolBarPlayer->setVisible(bShow);
  if(!bShow){
    delete player;
    player=NULL;
    return;
  }
  player=new TelemetryPlayer(_db,plot);
  player->setTime(plot->timeCursorValue());
  connect(player,&TelemetryPlayer::timeChanged,this,&TelemetryFrame::playerTimeChanged);
  connect(aPlay,&QAction::triggered,player,&TelemetryPlayer::play);
  connect(aPause,&QAction::triggered,player,&TelemetryPlayer::pause);
  connect(aRewind,&QAction::triggered,player,&TelemetryPlayer::rewind);
  playerSlider->setMaximum(recTimeMax);
  playerSpeed->setValue(1);
  playerTimeChanged();
}
//=============================================================================
void TelemetryFrame::playerSliderMoved()
{
  if(!player)return;
  player->setTime(playerSlider->value());
}
//=============================================================================
void TelemetryFrame::plotTimeCursorMoved()
{
  if(!player)return;
  player->setTime(plot->timeCursorValue());
}
//=============================================================================
void TelemetryFrame::playerTimeChanged()
{
  quint64 t=player->time();
  playerSlider->setValue(t);
  plot->setTimeCursor(t);
  lbPlayerTime->setText(FactSystem::timemsToString(t));
}
//=============================================================================



//=============================================================================
//=============================================================================
//=============================================================================
void TelemetryFrame::export_csv(QString fileName)
{
  /*QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, QApplication::applicationName(),QString(tr("Cannot write file")+" %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }
  QTextStream out(&file);
  //write file header..
  out << "time,"+QStringList(Vehicles::instance()->f_local->f_mandala->names).join(",")+QString("\n");

  uint cnt=0,i=0;
  foreach(uint time,Vehicles::instance()->f_local->f_recorder->file.time){
    if(((cnt++)&0x00FF)==0){
      QCoreApplication::processEvents();
      progress.setValue(time);
    }
    if(progress.wasCanceled()) {
      out.flush();
      file.close();
      file.remove();
      return;
    }
    const VehicleRecorder::ListDouble &vlist=Vehicles::instance()->f_local->f_recorder->file.data.at(i++);
    QStringList slist;
    slist.append(QString::number(time));
    foreach(double v,vlist){
      QString s=QString("%1").arg(v,0,'f',10);
      while(s.at(s.size()-1)=='0'){ //remove trailing zeros
        s.remove(s.size()-1,1);
        if(s.at(s.size()-1)!='.')continue;
        s.remove(s.size()-1,1);
        break;
      }
      slist.append(s);
    }
    out << slist.join(",")+QString("\n");
  }
  out.flush();
  file.close();*/
}
//------------------------------------------------------------------------
void TelemetryFrame::export_fdr(QString fileName)
{
  /*QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, QApplication::applicationName(),QString(tr("Cannot write file")+" %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }
  QTextStream out(&file);
  out << "A\n1\n\n";
  out << "ACFT,"    << "Aircraft/General Aviation/Cirrus TheJet/c4.acf"  << "\n";
  out << "TAIL,"    << "UAVOS.COM" << "\n";
  QDateTime t=QFileInfo(fileName).lastModified();
  t.setTime(QTime(12,0));
  out << "TIME,"    << t.toString("hh:mm:ss") << "\n";
  out << "DATE,"    << t.toString("dd/MM/yy") << "\n";
  out << "PRES,"    << "29.92" << "\n";
  out << "TEMP,"    << "65" << "\n";
  out << "WIND,"    << "230,17" << "\n";

  out << "\n\n";
  //export data...
  //read config
  QSettings st(AppDirs::res().filePath("preferences/xplane-fdr.conf"),QSettings::IniFormat);
  uint colCount=st.value("columns").toUInt();
  st.beginGroup("columns");
  QMap<int,QString> map;
  foreach(QString vname,st.childKeys()){
    if(Vehicles::instance()->f_local->f_mandala->names.contains(vname))
      map[st.value(vname).toUInt()]=vname;
  }
  st.endGroup();

  QProgressDialog progress(tr("Exporting telemetry file..."),tr("Abort"), 0, Vehicles::instance()->f_local->f_recorder->file.time.last());
  progress.setWindowModality(Qt::WindowModal);

  uint cnt=0,i=0;
  foreach(uint time,Vehicles::instance()->f_local->f_recorder->file.time){
    if(((cnt++)&0x00FF)==0){
      QCoreApplication::processEvents();
      progress.setValue(time);
    }
    if(progress.wasCanceled()) {
      out.flush();
      file.close();
      file.remove();
      return;
    }

    const VehicleRecorder::ListDouble &vlist=Vehicles::instance()->f_local->f_recorder->file.data.at(i++);
    out << "DATA,";
    for (uint iv=0;iv<colCount;iv++) {
      if(map.contains(iv)){
        out << QString::number(vlist.at(Vehicles::instance()->f_local->f_mandala->names.indexOf(map.value(iv))),'f',8);
      }else{
        out << QString::number(0);
      }
      out << ",";
    }
    out << "\n";
  }
  out.flush();
  file.close();*/
}
//------------------------------------------------------------------------
void TelemetryFrame::export_kml(QString fileName)
{
  /*QFile f(fileName);
  if (!f.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, QApplication::applicationName(),QString(tr("Cannot write file")+" %1:\n%2.").arg(fileName).arg(f.errorString()));
    return;
  }
  QNetworkAccessManager network;
  uint port = QSettings().value("httpServerPort").toUInt();
  QNetworkReply *reply = network.get(QNetworkRequest(QUrl(QString("http://127.0.0.1:%1/kml/telemetry")
                                   .arg(port))));
  reply->waitForReadyRead(10000);
  f.write(reply->readAll());
  f.flush();
  f.close();
  delete reply;*/
}
//=============================================================================
//=============================================================================








