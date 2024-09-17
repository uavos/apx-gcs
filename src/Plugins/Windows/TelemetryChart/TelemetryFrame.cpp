/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "TelemetryFrame.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <ApxMisc/MaterialIcon.h>
#include <ApxMisc/QActionFact.h>
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/TelemetryShare.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>
#include <QColor>
#include <QtGui>
#include <QtNetwork>

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
    connect(lookup, &LookupTelemetry::valueChanged, this, &TelemetryFrame::updateStats);

    connect(reader, &TelemetryReader::statsAvailable, this, &TelemetryFrame::updateStats);
    connect(reader, &TelemetryReader::dataAvailable, this, &TelemetryFrame::updateData);
    connect(telemetry, &TelemetryReader::progressChanged, this, &TelemetryFrame::updateProgress);
    connect(telemetry, &TelemetryReader::descrChanged, this, &TelemetryFrame::updateStatus);

    if (!QSettings().contains("Qwt_AntiAliased"))
        QSettings().setValue("Qwt_AntiAliased", false);

    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setContentsMargins(0, 0, 0, 0);
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
    toolBarSW->layout()->setContentsMargins(0, 0, 0, 0);
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

    const uint8_t calc_count = 4;
    for (int i = 0; i < calc_count; i++) {
        QwtPlotCurve *calc_curve = plot->addCurve(QString("calculated_%0").arg(i + 1),
                                                  QString("Calculated_%0 JS script value").arg(i + 1),
                                                  "",
                                                  QColor(Qt::yellow).lighter());
        plot->push_calc_curve(calc_curve);
    }

    connect(plot, &TelemetryPlot::itemVisibleChanged, this, [this](QwtPlotItem *item) {
        if (!plot->get_calc_curves().contains(static_cast<QwtPlotCurve *>(item))) {
            plot->saveSettings();
        }
    });

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

void TelemetryFrame::updateStats()
{
    //update title
    QString s = reader->title();
    s.append(QString(" (%1)").arg(reader->value().toString()));
    if (!reader->descr().isEmpty())
        s.append(" | ").append(reader->descr());
    quint64 recSize = reader->totalSize();
    if (recSize) {
        QString srcnt = recSize > 1000000 ? QString("%1M").arg(recSize / 1000000)
                        : recSize > 1000  ? QString("%1K").arg(recSize / 1000)
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
                         .arg(lookup->value().toString())
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

void TelemetryFrame::updateData()
{
    resetPlot();
    //create curves
    QHash<quint64, MandalaFact *> fidmap;
    for (auto fid : reader->fieldNames.keys()) {
        const QVector<QPointF> *d = reader->fieldData.value(fid);
        if (!d)
            continue;
        //check if all zero
        int zcnt = 0;
        for (auto const &p : *d) {
            if (p.y() != 0.0)
                break;
            zcnt++;
        }
        if (d->size() == zcnt)
            continue;

        const QString &s = reader->fieldNames.value(fid);
        MandalaFact *f = qobject_cast<MandalaFact *>(
            Vehicles::instance()->f_replay->f_mandala->findChild(s));
        if (!f)
            continue;
        fidmap.insert(fid, f);
    }
    QList<quint64> fidlist = fidmap.keys();
    std::sort(fidlist.begin(), fidlist.end(), [fidmap](quint64 v1, quint64 v2) {
        return fidmap.value(v1)->uid() < fidmap.value(v2)->uid();
    });

    ctr_fields.clear();
    plot->resetLegend();
    QHash<quint64, QwtPlotCurve *> cmap;
    for (auto fid : fidlist) {
        //map a fact
        const QString &s = reader->fieldNames.value(fid);
        const Fact *f = fidmap.value(fid);
        //fill params
        Qt::PenStyle style = Qt::SolidLine;
        if (s.startsWith("cmd."))
            style = Qt::DotLine;
        else if (s.contains(".gps.") || s.contains(".pos."))
            style = Qt::DashLine;
        else if (s.contains(".rc."))
            style = Qt::DotLine;
        QColor c(f->opts().value("color", QColor(Qt::white)).value<QColor>());
        QwtPlotCurve *cv = plot->addCurve(s, f->descr(), f->units(), QPen(c, 0, style));
        cmap.insert(fid, cv);
        if (s.startsWith("ctr."))
            ctr_fields.append(s);
    }

    plot->restoreSettings();

    //load data to plot
    for (auto fid : cmap.keys()) {
        cmap.value(fid)->setSamples(*reader->fieldData.value(fid));
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

void TelemetryFrame::resetPlot()
{
    if (aReplay->isChecked())
        aReplay->trigger();
    if (aShowEvents->isChecked())
        aShowEvents->trigger();
    plot->resetData();
}

void TelemetryFrame::eNotes_returnPressed(void)
{
    reader->f_notes->setValue(eNotes->text().trimmed());
    plot->setFocus();
}

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

void TelemetryFrame::aShowEvents_triggered(void)
{
    plot->setEventsVisible(aShowEvents->isChecked());
    if (pcopy)
        pcopy->setEventsVisible(aShowEvents->isChecked());
}

void TelemetryFrame::avCLR_triggered(void)
{
    plot->showCurves(false);
}
void TelemetryFrame::avSTD_triggered(void)
{
    QStringList st;
    st << "est.att.roll"
       << "cmd.att.roll"
       << "est.att.pitch"
       << "cmd.att.pitch";
    st << "est.pos.altitude";
    st << "est.air.airspeed"
       << "cmd.pos.airspeed";
    st << "est.pos.vspeed";
    plot->showCurves(true, st, true);
}
void TelemetryFrame::avIMU_triggered(void)
{
    QStringList st;
    st << "est.acc.x"
       << "est.acc.y"
       << "est.acc.z";
    st << "est.gyro.x"
       << "est.gyro.y"
       << "est.gyro.z";
    plot->showCurves(true, st, true);
}
void TelemetryFrame::avCTR_triggered(void)
{
    plot->showCurves(true, ctr_fields, true);
}

void TelemetryFrame::aReplay_triggered(void)
{
    bool bShow = aReplay->isChecked();
    if (bShow && (!reader->totalTime())) {
        aReplay->trigger();
        return;
    }
    toolBarPlayer->setVisible(bShow);
    if (!bShow) {
        player->f_stop->trigger();
        return;
    }
    playerSlider->setMaximum(reader->totalTime());
    playerSpeed->setValue(player->f_speed->value().toDouble());

    playerTimeChanged();
}

void TelemetryFrame::playerSliderMoved()
{
    player->f_time->setValue(playerSlider->value());
}

void TelemetryFrame::plotTimeCursorMoved()
{
    player->f_time->setValue(plot->timeCursorValue());
}

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
