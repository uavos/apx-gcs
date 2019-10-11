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
#include <App/App.h>
#include <App/AppDirs.h>
#include <ApxMisc/MaterialIcon.h>
#include <ApxMisc/QActionFact.h>
#include <Telemetry/LookupTelemetry.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>
#include <QtGui>
#include <QtNetwork>
//=============================================================================
TelemetryFrame::TelemetryFrame(QWidget *parent)
    : QWidget(parent)
    , pcopy(nullptr)
{
    //setWindowTitle(tr("Telemetry"));
    //setWindowFlags(Qt::Dialog|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);

    telemetry = Vehicles::instance()->f_replay->f_telemetry;
    lookup = telemetry->f_lookup;
    reader = telemetry->f_reader;
    player = telemetry->f_player;
    share = telemetry->f_share;

    connect(lookup, &LookupTelemetry::recordIdChanged, this, &TelemetryFrame::resetPlot);
    connect(lookup, &LookupTelemetry::statusChanged, this, &TelemetryFrame::updateStats);

    connect(reader, &TelemetryReader::statsAvailable, this, &TelemetryFrame::updateStats);
    connect(reader, &TelemetryReader::dataAvailable, this, &TelemetryFrame::updateData);
    connect(telemetry, &TelemetryReader::progressChanged, this, &TelemetryFrame::updateProgress);
    connect(telemetry, &TelemetryReader::descrChanged, this, &TelemetryFrame::updateStatus);

    if (!QSettings().contains("Qwt_AntiAliased"))
        QSettings().setValue("Qwt_AntiAliased", false);

    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    vlayout->addWidget(toolBar);

    toolBarPlayer = new QToolBar(this);
    toolBarPlayer->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBarPlayer->setVisible(false);
    vlayout->addWidget(toolBarPlayer);

    plot = new TelemetryPlot(this);
    plot->resetZoom();
    vlayout->addWidget(plot);

    lbTitle = new QLabel(this);
    lbTitle->setParent(plot);
    lbTitle->setStyleSheet("background-color: rgba(0,0,0,30%)");
    lbTitle->setAttribute(Qt::WA_TransparentForMouseEvents);

    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setStyleSheet(
        "QProgressBar {height: 10px; text-align: center; border: 1px solid gray;padding: "
        "0px;background: black;} QProgressBar::chunk {background: gray;border: 0px;}");
    progressBar->setVisible(false);
    progressBar->setParent(plot);
    progressBar->adjustSize();
    progressBar->resize(400, progressBar->height());
    //progressBar->move(2,lbTitle->height()+2);

    lbStatus = new QLabel(this);
    lbStatus->setParent(plot);
    lbStatus->setStyleSheet("background-color: rgba(0,0,0,30%)");
    lbStatus->setAttribute(Qt::WA_TransparentForMouseEvents);

    //actions
    QAction *a;
    toolBar->addAction(new QActionFact(lookup->f_latest));
    toolBar->addSeparator();
    toolBar->addAction(new QActionFact(lookup->f_prev));
    toolBar->addAction(new QActionFact(lookup->f_next));
    toolBar->addSeparator();
    a = new QActionFact(lookup);
    toolBar->addAction(a);
    toolBar->addSeparator();
    aSplit = toolBar->addAction(MaterialIcon("book-open-variant"),
                                tr("Split view"),
                                this,
                                &TelemetryFrame::aSplit_triggered);
    aSplit->setCheckable(true);
    aShowEvents = toolBar->addAction(MaterialIcon("ray-vertex"),
                                     tr("Show events"),
                                     this,
                                     &TelemetryFrame::aShowEvents_triggered);
    aShowEvents->setCheckable(true);
    toolBar->addSeparator();

    toolBar->addAction(new QActionFact(reader->f_reload));
    toolBar->addAction(new QActionFact(share->f_export));
    toolBar->addAction(new QActionFact(share->f_import));
    toolBar->addSeparator();
    toolBar->addAction(new QActionFact(lookup->f_remove));
    toolBar->addSeparator();

    aReplay = toolBar->addAction(MaterialIcon(player->icon()),
                                 tr("Replay"),
                                 this,
                                 &TelemetryFrame::aReplay_triggered);
    aReplay->setCheckable(true);
    toolBar->addSeparator();

    eNotes = new QLineEdit(this);
    eNotes->setPlaceholderText(tr("Notes"));
    connect(eNotes, &QLineEdit::returnPressed, this, &TelemetryFrame::eNotes_returnPressed);
    toolBar->addWidget(eNotes);

    toolBarSW = new QToolBar(this);
    toolBarSW->setObjectName("toolBarSW");
    toolBarSW->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBarSW->layout()->setMargin(0);
    toolBar->addWidget(toolBarSW);
    avCLR = toolBarSW->addAction(tr("CLR"), this, &TelemetryFrame::avCLR_triggered);
    avSTD = toolBarSW->addAction(tr("STD"), this, &TelemetryFrame::avSTD_triggered);
    avIMU = toolBarSW->addAction(tr("IMU"), this, &TelemetryFrame::avIMU_triggered);
    avCTR = toolBarSW->addAction(tr("CTR"), this, &TelemetryFrame::avCTR_triggered);

    //player
    toolBarPlayer->addAction(new QActionFact(player->f_play));
    toolBarPlayer->addAction(new QActionFact(player->f_stop));
    toolBarPlayer->addAction(new QActionFact(player->f_rewind));
    playerSpeed = new QDoubleSpinBox(this);
    toolBarPlayer->addWidget(playerSpeed);
    playerSpeed->setMinimum(0.100);
    playerSpeed->setMaximum(100);
    playerSpeed->setSingleStep(0.1);
    playerSpeed->setAccelerated(true);
    playerSpeed->setDecimals(3);
    connect(playerSpeed,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            player->f_speed,
            [=](double v) { player->f_speed->setValue(v); });
    playerSlider = new QSlider(Qt::Horizontal, this);
    toolBarPlayer->addWidget(playerSlider);
    connect(playerSlider, &QSlider::sliderMoved, this, &TelemetryFrame::playerSliderMoved);
    connect(plot, &TelemetryPlot::timeCursorChanged, this, &TelemetryFrame::plotTimeCursorMoved);
    lbPlayerTime = new QLabel(this);
    lbPlayerTime->setFont(App::getMonospaceFont());
    toolBarPlayer->addWidget(lbPlayerTime);
    plotCursorUpdateTimer.setSingleShot(true);
    connect(&plotCursorUpdateTimer, &QTimer::timeout, this, &TelemetryFrame::updatePlotPlayerTime);

    connect(player->f_time, &Fact::valueChanged, this, &TelemetryFrame::playerTimeChanged);
    //connect(player,&Fact::activeChanged,this,&TelemetryFrame::playerStateChanged);

    //create plot fields
    foreach (VehicleMandalaFact *f, Vehicles::instance()->f_replay->f_mandala->allFacts) {
        //fill params
        QString sn = f->name();
        Qt::PenStyle style = Qt::SolidLine;
        if (sn.contains("cmd_"))
            style = Qt::DotLine;
        else if (sn.contains("gps_"))
            style = Qt::DashLine;
        else if (sn.contains("rc_"))
            style = Qt::DotLine;
        QColor c(f->opts().value("color", QColor(Qt::white)).value<QColor>());
        QwtPlotCurve *cv = plot->addCurve(sn, f->descr(), f->units(), QPen(c, 0, style));
        plotMap.insert(cv, f);
    }
    plot->calc = plot->addCurve("calculated",
                                tr("Calculated user variable"),
                                "",
                                QColor(Qt::yellow).lighter());
    plot->restoreSettings();
    connect(plot, &TelemetryPlot::itemVisibleChanged, this, [=]() { plot->saveSettings(); });

    //update css styles
    foreach (QAction *a, toolBar->actions()) {
        toolBar->widgetForAction(a)->setObjectName(a->objectName());
    }
    foreach (QAction *a, toolBarPlayer->actions()) {
        toolBarPlayer->widgetForAction(a)->setObjectName(a->objectName());
    }

    updateStats();
    updateData();
    updateProgress();
    updateStatus();
}
//=============================================================================
//=============================================================================
void TelemetryFrame::updateStats()
{
    //update title
    QString s = reader->title();
    s.append(QString(" (%1)").arg(reader->status()));
    if (!reader->descr().isEmpty())
        s.append(" | ").append(reader->descr());
    quint64 recSize = reader->totalSize();
    if (recSize) {
        QString srcnt = recSize > 1000000 ? QString("%1M").arg(recSize / 1000000)
                                          : recSize > 1000 ? QString("%1K").arg(recSize / 1000)
                                                           : QString("%1").arg(recSize);
        s.append(QString("\t(%1 %2)").arg(srcnt).arg(tr("records")));
    }
    //events stats
    QString recStats;
    QStringList st;
    foreach (QString s, reader->evtCountMap.keys()) {
        st.append(QString("%1: %2").arg(s).arg(reader->evtCountMap.value(s)));
    }
    recStats = st.join(" | ");
    //set label
    lbTitle->setText(QString("%1    \t%2%3")
                         .arg(lookup->status())
                         .arg(s)
                         .arg(recStats.isEmpty() ? recStats : recStats.prepend("\n\t")));
    lbTitle->adjustSize();
    progressBar->move(2, 2 + lbTitle->geometry().height() + 5);
    updateStatus();
    setEnabled(true);
    eNotes->setText(reader->f_notes->text());
}
void TelemetryFrame::updateProgress()
{
    int v = telemetry->progress();
    progressBar->setMaximum(v == 0 ? 0 : 100);
    progressBar->setValue(v);
    progressBar->setVisible(v >= 0);
}
void TelemetryFrame::updateStatus()
{
    lbStatus->setText(telemetry->descr());
    lbStatus->setVisible(progressBar->isVisible());
    QRect r = progressBar->geometry();
    lbStatus->adjustSize();
    lbStatus->move(r.right() + 8, r.y() + r.height() / 2 - lbStatus->height() / 2);
}
//=============================================================================
//=============================================================================
void TelemetryFrame::updateData()
{
    //map facts by fieldNames to fid
    QHash<Fact *, quint64> fidMap;
    foreach (quint64 fid, reader->fieldNames.keys()) {
        const QString &s = reader->fieldNames.value(fid);
        Fact *f = Vehicles::instance()->f_replay->f_mandala->factByName(s);
        if (!f)
            continue;
        fidMap.insert(f, fid);
    }
    //load data to plot
    foreach (QwtPlotCurve *c, plotMap.keys()) {
        quint64 fid = fidMap.value(plotMap.value(c), 0);
        if (!fid)
            continue;
        QVector<QPointF> *d = reader->fieldData.value(fid);
        if (!d)
            continue;
        c->setSamples(*d);
    }
    //load events
    for (TelemetryReader::events_t::iterator e = reader->events.begin(); e != reader->events.end();
         ++e) {
        //const TelemetryReader::event_t &e = i;
        if (e->name == "msg")
            continue;
        if (e->name == "serial")
            continue;
        if (e->name == "xpdr")
            continue;
        QColor c;
        if (e->name == "mission")
            c = QColor(50, 50, 100);
        else if (e->name == "conf")
            c = QColor(100, 100, 50);
        else if (e->name == "uplink")
            c = QColor(Qt::darkCyan);
        plot->addEvent(e->time / 1000.0, QString("%1: %2").arg(e->name).arg(e->value), c);
    }

    plot->resetZoom();
}
//=============================================================================
//=============================================================================
void TelemetryFrame::resetPlot()
{
    if (aReplay->isChecked())
        aReplay->trigger();
    if (aShowEvents->isChecked())
        aShowEvents->trigger();
    plot->resetData();
}
//=============================================================================
void TelemetryFrame::aFilter_triggered(void)
{
    /*if(aFilter->isChecked()){
    aFilter->setChecked(!reader->f_filter->text().isEmpty());
    //rescan();
    bool ok;
    QString item=QInputDialog::getItem(nullptr,aFilter->toolTip(),aFilter->text(),reader->f_filter->enumStrings(),0,false,&ok);
    if(!ok) return;
    reader->f_filter->setValue(item);
  }else reader->f_filter->setValue("");
  aFilter->setChecked(!reader->f_filter->text().isEmpty());*/
}
//=============================================================================
void TelemetryFrame::eNotes_returnPressed(void)
{
    reader->f_notes->setValue(eNotes->text().trimmed());
    plot->setFocus();
}
//=============================================================================
void TelemetryFrame::aSplit_triggered(void)
{
    if (!((QAction *) sender())->isChecked()) {
        vlayout->removeWidget(pcopy);
        if (pcopy)
            pcopy->deleteLater();
        pcopy = nullptr;
        return;
    }
    pcopy = new TelemetryPlot();
    pcopy->copyFromPlot(plot);
    vlayout->addWidget(pcopy);
    //connect(pcopy,&TelemetryPlot::progressChanged,this,&TelemetryFrame::setProgress);
}
//=============================================================================
void TelemetryFrame::aShowEvents_triggered(void)
{
    plot->setEventsVisible(aShowEvents->isChecked());
    if (pcopy)
        pcopy->setEventsVisible(aShowEvents->isChecked());
}
//=============================================================================
void TelemetryFrame::avCLR_triggered(void)
{
    plot->showCurves(false);
}
void TelemetryFrame::avSTD_triggered(void)
{
    QStringList st;
    st << "roll"
       << "cmd_roll"
       << "pitch"
       << "cmd_pitch";
    st << "altitude"; //<<"cmd_altitude";
    st << "airspeed"
       << "cmd_airspeed";
    st << "vspeed";
    plot->showCurves(true, st, true);
}
void TelemetryFrame::avIMU_triggered(void)
{
    QStringList st;
    st << "Ax"
       << "Ay"
       << "Az";
    st << "p"
       << "q"
       << "r";
    st << "Hx"
       << "Hy"
       << "Hz";
    plot->showCurves(true, st, true);
}
void TelemetryFrame::avCTR_triggered(void)
{
    QStringList st;
    foreach (QString vn, Vehicles::instance()->f_local->f_mandala->names)
        if (vn.startsWith("ctr"))
            st.append(vn);
    plot->showCurves(true, st, true);
}
//=============================================================================
//=============================================================================
//=============================================================================
void TelemetryFrame::aReplay_triggered(void)
{
    bool bShow = aReplay->isChecked();
    if (bShow && (!reader->totalTime())) {
        aReplay->trigger();
        return;
    }
    toolBarPlayer->setVisible(bShow);
    if (!bShow) {
        player->stop();
        return;
    }
    playerSlider->setMaximum(reader->totalTime());
    playerSpeed->setValue(player->f_speed->value().toDouble());

    playerTimeChanged();
}
//=============================================================================
void TelemetryFrame::playerSliderMoved()
{
    player->f_time->setValue(playerSlider->value());
}
//=============================================================================
void TelemetryFrame::plotTimeCursorMoved()
{
    player->f_time->setValue(plot->timeCursorValue());
}
//=============================================================================
void TelemetryFrame::playerTimeChanged()
{
    if (plotCursorUpdateTimer.isActive())
        return;
    plotCursorUpdateTimer.start(100);
}
void TelemetryFrame::updatePlotPlayerTime()
{
    quint64 t = player->f_time->value().toULongLong();
    playerSlider->setValue(t);
    plot->setTimeCursor(t, reader->totalSize() < 3000000);
    lbPlayerTime->setText(AppRoot::timemsToString(t));
}
//=============================================================================

//=============================================================================
//=============================================================================
//=============================================================================
void TelemetryFrame::export_csv(QString fileName)
{
    Q_UNUSED(fileName)
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
    Q_UNUSED(fileName)
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
    Q_UNUSED(fileName)
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
