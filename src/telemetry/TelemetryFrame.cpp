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
#include "QMandala.h"
#include "QMandala.h"
#include <QtGui>
#include <QtNetwork>
#include <QDomDocument>
//=============================================================================
TelemetryFrame::TelemetryFrame(QWidget *parent)
  :QWidget(parent)
{
  setupUi(this);

  if(!QSettings().contains("Qwt_AntiAliased"))QSettings().setValue("Qwt_AntiAliased",false);
  QToolBar *toolBar=new QToolBar(this);
  toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  toolBarLayout->insertWidget(1,toolBar);

  toolBar->addAction(aLast);
  toolBar->addAction(aReload);
  toolBar->addSeparator();
  toolBar->addAction(aPrev);
  toolBar->addAction(aNext);
  toolBar->addSeparator();
  toolBar->addAction(aPlay);
  toolBar->addSeparator();
  toolBar->addAction(aDelete);
  toolBar->addSeparator();
  toolBar->addAction(aFullScreen);
  toolBar->addAction(aSplitV);
  toolBar->addSeparator();
  toolBar->addAction(aEdit);
  toolBar->addAction(aExport);
  toolBar->addAction(aReport);
  toolBar->addAction(aFiles);
  //toolBar->addSeparator();
  //toolBar->addAction(aCut);
  toolBar->addSeparator();
  toolBar->addAction(aFilterUAV);
  toolBar->addSeparator();

  toolBar=new QToolBar(this);
  toolBar->setObjectName("toolBarSW");
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toolBar->setIconSize(QSize(16,16));
  toolBar->layout()->setMargin(0);
  toolBarLayout->insertWidget(3,toolBar);
  toolBar->addAction(avCLR);
  toolBar->addAction(avSTD);
  toolBar->addAction(avIMU);
  toolBar->addAction(avCTR);

  //player
  connect(&player,SIGNAL(timeTrack(uint)),&plot,SLOT(timeTrack(uint)));
  /*QToolBar *tbPlayer=new QToolBar(this);
  tbPlayer->setIconSize(QSize(16,16));
  tbPlayer->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  tbPlayer->layout()->setMargin(0);
  verticalLayout->insertWidget(1,tbPlayer);
  tbPlayer->addAction(aLast);
  tbPlayer->addAction(aReload);*/


  //plot
  QVBoxLayout *layout=new QVBoxLayout(widget);
  layout->setMargin(0);
  widget->setLayout(layout);
  layout->addWidget(&plot);

  QTimer::singleShot(500,this,SLOT(loadRecent()));
}
//=============================================================================
void TelemetryFrame::loadRecent()
{
  rescan();
  QString telemetryFile=QSettings().value("telemetryFile").toString();
  if (telemetryFile.isEmpty()||(!filesList.contains(telemetryFile))) aLast->trigger();
  else load(filesList.indexOf(telemetryFile));
}
//=============================================================================
TelemetryFrame::~TelemetryFrame()
{
}
//=============================================================================
void TelemetryFrame::on_aPrev_triggered(void)
{
  if(flightNo>0)load(flightNo-1);
}
void TelemetryFrame::on_aNext_triggered(void)
{
  if(flightNo<(flightCnt-1))load(flightNo+1);
}
void TelemetryFrame::on_aLast_triggered(void)
{
  rescan();
  load(flightCnt-1);
}
void TelemetryFrame::on_aReload_triggered(void)
{
  rescan();
  load(flightNo);
}
//=============================================================================
void TelemetryFrame::on_aDelete_triggered(void)
{
  QMandala::instance()->local->rec->close();
  QProcess p;
  QStringList arg=QStringList()<<"move"<<QUrl().fromLocalFile(QMandala::instance()->local->rec->loadFile.fileName()).toString()<<"trash:/";
  p.start("kioclient",arg);
  if(p.waitForFinished(5000)) qDebug(tr("File '%s' moved to trash.").toUtf8().data(),QFileInfo(QMandala::instance()->local->rec->loadFile).baseName().toUtf8().data());
  else{
    qDebug(tr("File '%s' deleted permanently.").toUtf8().data(),QFileInfo(QMandala::instance()->local->rec->loadFile).baseName().toUtf8().data());
    QMandala::instance()->local->rec->loadFile.remove();
    }
  rescan();
  if(flightCnt) load(flightNo>=flightCnt?flightCnt-1:flightNo);
  else load(0);
}
//=============================================================================
void TelemetryFrame::rescan(void)
{
  QMandala::instance()->local->rec->flush();
  filesList=QMandala::instance()->local->rec->recFileNames();
  //find UAV names..
  uavNames.clear();
  foreach(const QString &fname,filesList){
    QString s=QFileInfo(fname).baseName();
    if(!s.contains("_"))continue;
    s=s.right(s.size()-s.indexOf('_',s.indexOf('_',s.indexOf('_',s.indexOf('_',s.indexOf('_',s.indexOf('_')+1)+1)+1)+1)+1)-1);
    if(!uavNames.contains(s))uavNames.append(s);
  }
  uavNames.sort();
  //filter list
  filteredList=filesList.filter(filter);

  flightCnt=filteredList.size();
  aExport->setEnabled(false);
  aDelete->setEnabled(flightCnt);
}
//=============================================================================
void TelemetryFrame::on_aFilterUAV_triggered(void)
{
  if(aFilterUAV->isChecked()){
    aFilterUAV->setChecked(!filter.isEmpty());
    bool ok;
    QString item=QInputDialog::getItem(this,aFilterUAV->toolTip(),aFilterUAV->text(),uavNames,0,false,&ok);
    if(!ok) return;
    filter=item;
  }else filter="";
  aFilterUAV->setChecked(!filter.isEmpty());
  aLast->trigger();
}
//=============================================================================
void TelemetryFrame::on_lbCurrent_clicked(void)
{
  bool ok;
  int v=QInputDialog::getInt(
      this,QApplication::applicationName(),
      tr("Flight number to load"),flightNo+1,1,flightCnt,1,&ok);
  if(ok)load(v-1);
}
//=============================================================================
void TelemetryFrame::on_aPlay_triggered(void)
{
  player.showNormal();
  player.raise();
}
//=============================================================================
//=============================================================================
void TelemetryFrame::load(int idx)
{
  if(idx<0 || idx>=filteredList.size()) return;
  int i=filesList.indexOf(filteredList.at(idx));
  setEnabled(false);
  plot.load(i);
  flightNo=idx;
  lbCurrent->setText(QString("%1").arg(flightNo+1));
  lbTotal->setText(QString("%1").arg(flightCnt));

  aExport->setEnabled(true);
  aNext->setEnabled((flightNo+1)<flightCnt);
  aPrev->setEnabled(flightNo>0);
  setEnabled(true);
  eNotes->setText(QMandala::instance()->local->rec->file.notes);
}
//=============================================================================
//=============================================================================
void TelemetryFrame::on_eNotes_returnPressed(void)
{
  QDomDocument doc("telemetry");
  QFile file(QMandala::Global::records().filePath(filesList.at(flightNo)));
  if(!file.open(QIODevice::ReadWrite|QIODevice::Text))return;
  QString err;
  if(!doc.setContent(&file,&err)){
    qDebug()<<err;
    file.close();
    return;
  }
  QDomElement root = doc.firstChildElement();
  QDomElement notes = root.firstChildElement("notes");
  if(notes.isNull()){
    notes=doc.createElement("notes");
    root.appendChild(notes);
    notes.appendChild(doc.createTextNode(""));
  }
  notes.firstChild().setNodeValue(eNotes->text());
  //qDebug()<<notes;
  file.resize(0);
  QTextStream stream(&file);
  stream.setDevice(&file);
  doc.save(stream,0);
  file.close();
  qDebug("%s",tr("Telemetry file updated.").toUtf8().data());
  eNotes->clearFocus();
}
//=============================================================================
//=============================================================================
void TelemetryFrame::on_aSplitV_triggered(void)
{
  if(!((QAction*)sender())->isChecked()){
    widget->layout()->removeWidget(pcopy);
    delete pcopy;
    return;
  }
  pcopy=new TelemetryPlot();
  pcopy->copyFromPlot(&plot);
  widget->layout()->addWidget(pcopy);
}
//=============================================================================
void TelemetryFrame::on_avCLR_triggered(void)
{
  plot.showCurves(false);
}
void TelemetryFrame::on_avSTD_triggered(void)
{
  QStringList st;
  st<<"roll"<<"cmd_roll"<<"pitch"<<"cmd_pitch";
  st<<"altitude"<<"cmd_altitude";
  st<<"airspeed"<<"cmd_airspeed";
  st<<"vspeed";
  plot.showCurves(true,st,true);
}
void TelemetryFrame::on_avIMU_triggered(void)
{
  QStringList st;
  st<<"Ax"<<"Ay"<<"Az";
  st<<"p"<<"q"<<"r";
  st<<"Hx"<<"Hy"<<"Hz";
  plot.showCurves(true,st,true);
}
void TelemetryFrame::on_avCTR_triggered(void)
{
  QStringList st;
  foreach (QString vn,QMandala::instance()->local->names)
    if(vn.startsWith("ctr"))st.append(vn);
  plot.showCurves(true,st,true);
}
//=============================================================================
void TelemetryFrame::on_aCut_triggered(void)
{/*
  QwtScaleDiv *sx=plot.axisScaleDiv(QwtPlot::xBottom);
  double stime=plot.time_start+sx->lowerBound();
  double etime=plot.time_start+sx->upperBound();
  QFile f(QMandala::instance()->local->rec->loadFile.fileName());
  f.open(QIODevice::ReadOnly);
  QTextStream stream(&f);
  QStringList dest;
  dest.append(stream.readLine());
  while (1) {
    QString line=stream.readLine();
    if (line.isNull())break;
    QStringList vlist=line.split(",");
    double time=vlist.at(0).toUInt()/1000.0;
    if(time>etime)break;
    if(time<stime)continue;
    dest.append(line);
  }
  f.close();
  f.open(QIODevice::WriteOnly);
  f.write(dest.join("\n").toUtf8().data());
  f.close();
  load(flightNo);
  qDebug("Cut: %s",QFileInfo(f).baseName().toUtf8().data());*/
}
//=============================================================================
//=============================================================================
void TelemetryFrame::on_aExport_triggered(void)
{
  if (!QMandala::instance()->local->rec->file.time.size())return;
  QString fileName(QMandala::instance()->local->rec->loadFile.fileName());

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
  qDebug("#%s.",tr("File exported").toUtf8().data());
}
//------------------------------------------------------------------------
void TelemetryFrame::export_csv(QString fileName)
{
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, QApplication::applicationName(),QString(tr("Cannot write file")+" %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }
  QTextStream out(&file);
  //write file header..
  out << "time,"+QStringList(QMandala::instance()->local->names).join(",")+QString("\n");

  QProgressDialog progress(tr("Exporting telemetry file..."),tr("Abort"), 0, QMandala::instance()->local->rec->file.time.last());
  progress.setWindowModality(Qt::WindowModal);

  uint cnt=0,i=0;
  foreach(uint time,QMandala::instance()->local->rec->file.time){
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
    const FlightDataFile::ListDouble &vlist=QMandala::instance()->local->rec->file.data.at(i++);
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
  file.close();
}
//------------------------------------------------------------------------
void TelemetryFrame::export_fdr(QString fileName)
{
  QFile file(fileName);
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
  QSettings st(QMandala::Global::config().filePath("xplane-fdr.conf"),QSettings::IniFormat);
  uint colCount=st.value("columns").toUInt();
  st.beginGroup("columns");
  QMap<int,QString> map;
  foreach(QString vname,st.childKeys()){
    if(QMandala::instance()->local->names.contains(vname))
      map[st.value(vname).toUInt()]=vname;
  }
  st.endGroup();

  QProgressDialog progress(tr("Exporting telemetry file..."),tr("Abort"), 0, QMandala::instance()->local->rec->file.time.last());
  progress.setWindowModality(Qt::WindowModal);

  uint cnt=0,i=0;
  foreach(uint time,QMandala::instance()->local->rec->file.time){
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

    const FlightDataFile::ListDouble &vlist=QMandala::instance()->local->rec->file.data.at(i++);
    out << "DATA,";
    for (uint iv=0;iv<colCount;iv++) {
      if(map.contains(iv)){
        out << QString::number(vlist.at(QMandala::instance()->local->names.indexOf(map.value(iv))),'f',8);
      }else{
        out << QString::number(0);
      }
      out << ",";
    }
    out << "\n";
  }
  out.flush();
  file.close();
}
//------------------------------------------------------------------------
void TelemetryFrame::export_kml(QString fileName)
{
  QFile f(fileName);
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
  delete reply;
}
//=============================================================================
void TelemetryFrame::on_aFiles_triggered(void)
{
  QDesktopServices::openUrl(QUrl("file://"+QMandala::Global::records().absoluteFilePath(".")));
}
void TelemetryFrame::on_aEdit_triggered(void)
{
  QProcess::startDetached("kate",QStringList()<<QMandala::instance()->local->rec->loadFile.fileName());
}
//=============================================================================
void TelemetryFrame::on_aFullScreen_triggered(void)
{
  if(windowState()==Qt::WindowFullScreen){
    setWindowFlags(Qt::Widget);
    showNormal();
  }else {
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
//=============================================================================
void TelemetryFrame::on_aReport_triggered(void)
{/*
  QFileInfo finfo(QMandala::instance()->local->rec->loadFile);

  QMap<QString,QString> data;

  data["num"]=QString::number(flightNo+1);
  data["now"]=QDateTime::currentDateTime().toString("ddd MMMM d yyyy hh:mm");

  //parse file name...
  QString bname=finfo.baseName();
  data["file"]=bname;
  //get date
  QDateTime date(QDateTime::fromString(bname.left(16),"yyyy_MM_dd_hh_mm"));
  data["date"]=date.toString("ddd MMMM d yyyy hh:mm");

  //get uav name
  QStringList stF(bname.split('_'));
  if(stF.size()<7){
    qDebug("Unable to parse filename.");
    return;
  }
  data["uav"]=stF.at(6);

  //get wpt cnt
  //data["wpt"]=QString::number(QMandala::instance()->local->wpcnt);


  //calc data
  uint timeStart=0,vTime=0;
  bool bFly=false,bFirst=true;
  double vAlt,alt=0,gcuAlt=0,vAltAGL=0,altAGL=0;
  double vTemp,tempMax=0,tempSum=0;
  double vBat,bat=0,batLow=0;
  double vSpd,spdMax=0,spdMin=0,spdTakeoff=0,spdTouch=0;
  double vRoll,maxRoll=0;
  double vPitch,maxPitch=0,minPitch=0;
  double vTurnRate,maxTurnRate=0;
  double vVspeed,vspeedUp=0,vspeedDown=0;
  double vDist,dist=0;
  double vAccX,vAccY,vAccZ,accX=0,accY=0,accZ=0;
  double homeLat=0,homeLon=0,vLat,vLon;
  uint cnt=0;
  foreach(uint time_ms,QMandala::instance()->local->rec->file.time) {
    double time=time_ms/1000.0;
    //-------------------
    // fetch data...
    vTime=time;
    vAlt=QMandala::instance()->local->rec->value("gps_hmsl",cnt);
    vBat=QMandala::instance()->local->rec->value("Vs",cnt);
    vTemp=QMandala::instance()->local->rec->value("AT",cnt);
    vSpd=QMandala::instance()->local->rec->value("airspeed",cnt);
    vVspeed=QMandala::instance()->local->rec->value("vspeed",cnt);
    vDist=QMandala::instance()->local->rec->value("dHome",cnt);
    vTurnRate=fabs(QMandala::instance()->local->rec->value("crsRate",cnt));
    Vect theta=Vect(
        QMandala::instance()->local->rec->value("theta[0]",cnt),
        QMandala::instance()->local->rec->value("theta[1]",cnt),
        QMandala::instance()->local->rec->value("theta[2]",cnt)
        );
    vRoll=fabs(theta[0]);
    vPitch=theta[1];
    Vect acc=Vect(
        QMandala::instance()->local->rec->value("aXYZ[0]",cnt),
        QMandala::instance()->local->rec->value("aXYZ[1]",cnt),
        QMandala::instance()->local->rec->value("aXYZ[2]",cnt)
        );
    vAccX=fabs(acc[0]);
    vAccY=fabs(acc[1]);
    vAccZ=fabs(acc[2]);
    vLat=QMandala::instance()->local->rec->value("gps_lat",cnt);
    vLon=QMandala::instance()->local->rec->value("gps_lon",cnt);

    cnt++;
    //-------------------



    tempSum+=vTemp;


    if(bFirst){
      bFirst=false;
      gcuAlt=vAlt;
      batLow=vBat;
      bat=vBat;
      tempMax=vTemp;
      //temp=vTemp;
      spdMax=vSpd;
      spdMin=vSpd;
      spdTakeoff=vSpd;
      spdTouch=vSpd;
      timeStart=vTime;
      homeLat=vLat;
      homeLon=vLon;
      continue;
    }

    vAltAGL=vAlt-gcuAlt;

    if(tempMax<vTemp)tempMax=vTemp;
    if(alt<vAlt)alt=vAlt;
    if(altAGL<vAltAGL)altAGL=vAltAGL;
    if(batLow>vBat)batLow=vBat;
    if(spdMax<vSpd)spdMax=vSpd;
    if(dist<vDist)dist=vDist;


    if(bFly){
      if(vAltAGL<5 && vAccZ>8){
        //touchdown
        bFly=false;
        spdTouch=vSpd;
        break;
      }
      if(vAltAGL>50){
        //during the flight
        if(spdMin>vSpd)spdMin=vSpd;
        if(maxTurnRate<vTurnRate)maxTurnRate=vTurnRate;
        if(maxRoll<vRoll)maxRoll=vRoll;
        if(maxPitch<vPitch)maxPitch=vPitch;
        if(minPitch>vPitch)minPitch=vPitch;
        if(vspeedUp<vVspeed)vspeedUp=vVspeed;
        if(vspeedDown>vVspeed)vspeedDown=vVspeed;
        if(accX<vAccX)accX=vAccX;
        if(accY<vAccY)accY=vAccY;
        if(accZ<vAccZ)accZ=vAccZ;
      }
    }else{
      if(vAltAGL>5){
        //takeoff
        bFly=true;
        spdTakeoff=vSpd;
       }
    }
  }

  //-------------------------------------------------
  // assign data to report
  uint itime=vTime-timeStart;
  QTime duration(itime/60/60,(itime/60)%60,itime%60);
  double fcnt=cnt;
  data["duration"]=duration.toString("hh:mm:ss");

  data["pos"]=QMandala::latToString(homeLat)+" "+QMandala::lonToString(homeLon);

  data["temp"]=QString("%1 deg C").arg(tempSum/fcnt,0,'f',1);
  data["tempMax"]=QString("%1 deg C").arg(tempMax,0,'f',1);

  data["gcuAlt"]=QString("%1 m").arg(gcuAlt,0,'f',0);
  data["alt"]=QString("%1 m").arg(alt,0,'f',0);
  data["altAGL"]=QString("%1 m").arg(altAGL,0,'f',0);

  data["bat"]=QString("%1 v").arg(bat,0,'f',1);
  data["batLow"]=QString("%1 v").arg(batLow,0,'f',1);

  data["spdMax"]=QString("%1 m/s").arg(spdMax,0,'f',0);
  data["spdMin"]=QString("%1 m/s").arg(spdMin,0,'f',0);
  data["spdTakeoff"]=QString("%1 m/s").arg(spdTakeoff,0,'f',0);
  data["spdTouch"]=QString("%1 m/s").arg(spdTouch,0,'f',0);

  data["maxRoll"]=QString("%1 deg").arg(maxRoll,0,'f',0);
  data["maxPitch"]=QString("%1 deg").arg(maxPitch,0,'f',0);
  data["minPitch"]=QString("%1 deg").arg(minPitch,0,'f',0);
  data["maxTurnRate"]=QString("%1 deg/s").arg(maxTurnRate,0,'f',0);

  data["vspeedUp"]=QString("%1 m/s").arg(vspeedUp,0,'f',0);
  data["vspeedDown"]=QString("%1 m/s").arg(fabs(vspeedDown),0,'f',0);

  data["dist"]=QString("%1 km").arg(dist/1000.0,0,'f',1);

  data["accX"]=QString("%1 g").arg(accX/9.81,0,'f',1);
  data["accY"]=QString("%1 g").arg(accY/9.81,0,'f',1);
  data["accZ"]=QString("%1 g").arg(accZ/9.81,0,'f',1);

  Report rep(&data);
  rep.exec();*/
}
//=============================================================================









