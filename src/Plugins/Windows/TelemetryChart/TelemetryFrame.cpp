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
#include <Telemetry/TelemetryRecords.h>
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

    auto vehicle = Vehicles::instance()->f_replay;
    mandala = vehicle->f_mandala;
    telemetry = vehicle->f_telemetry;
    records = telemetry->f_records;
    reader = telemetry->f_reader;
    player = telemetry->f_player;
    share = telemetry->f_share;

    connect(reader, &TelemetryReader::rec_started, this, &TelemetryFrame::rec_started);
    connect(reader, &TelemetryReader::rec_finished, this, &TelemetryFrame::rec_finished);
    connect(reader, &TelemetryReader::rec_field, this, &TelemetryFrame::rec_field);
    connect(reader, &TelemetryReader::rec_values, this, &TelemetryFrame::rec_values);
    connect(reader, &TelemetryReader::rec_evt, this, &TelemetryFrame::rec_evt);
    connect(reader, &TelemetryReader::rec_msg, this, &TelemetryFrame::rec_msg);
    connect(reader, &TelemetryReader::rec_meta, this, &TelemetryFrame::rec_meta);
    connect(reader, &TelemetryReader::rec_raw, this, &TelemetryFrame::rec_raw);

    connect(reader, &TelemetryReader::recordInfoChanged, this, &TelemetryFrame::updateStats);
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
    toolBar->addAction(new QActionFact(records->f_latest));
    toolBar->addSeparator();
    toolBar->addAction(new QActionFact(records->f_prev));
    toolBar->addAction(new QActionFact(records->f_next));
    toolBar->addSeparator();
    a = new QActionFact(records);
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
    toolBar->addAction(new QActionFact(records->f_remove));
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

    //set label
    lbTitle->setText(QString("%1    \t%2").arg(records->value().toString(), s));
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

void TelemetryFrame::rec_started()
{
    resetPlot();
    _fields.clear();
    _samples.clear();
    _timeMax = 0;
}
void TelemetryFrame::rec_finished()
{
    //final data tail at max time
    for (auto &pts : _samples) {
        if (pts.isEmpty()) {
            pts.append(QPointF(0, 0));
            //continue;
        }
        if (pts.last().x() >= _timeMax)
            continue;
        pts.append(QPointF(_timeMax, pts.last().y()));
    }

    // sort fields by name
    QMap<QString, int> fieldOrder;
    for (const auto &i : _fields) {
        fieldOrder.insert(i.name, fieldOrder.size());
    }

    // create plot fields
    ctr_fields.clear();
    plot->resetLegend();
    for (const auto [name, idx] : fieldOrder.asKeyValueRange()) {
        //map a fact
        const QString &s = name;
        //fill params
        Qt::PenStyle style = Qt::SolidLine;
        if (s.startsWith("cmd."))
            style = Qt::DotLine;
        else if (s.contains(".gps.") || s.contains(".pos."))
            style = Qt::DashLine;
        else if (s.contains(".rc."))
            style = Qt::DotLine;

        if (s.startsWith("ctr."))
            ctr_fields.append(s);

        // try to find matching fact to guess color
        const auto f = mandala->fact(s, true);
        QColor c(Qt::white);
        if (f) {
            c = f->opts().value("color", c).value<QColor>();
        }
        const auto &field = _fields.at(idx);
        auto curve = plot->addCurve(s, field.title, field.units, QPen(c, 0, style));

        // assign data to plot
        if (idx < _samples.size())
            curve->setSamples(_samples.value(idx));
    }

    plot->restoreSettings();
    plot->resetZoom();
}

void TelemetryFrame::rec_field(QString name, QString title, QString units)
{
    _fields.append({name, title, units});
}
void TelemetryFrame::rec_values(quint64 timestamp_ms, TelemetryReader::Values data, bool uplink)
{
    if (uplink) {
        // uplink goes to event
        for (const auto [idx, value] : data.asKeyValueRange()) {
            QString name = _fields.at(idx).name;
            plot->addEvent(timestamp_ms / 1000.0,
                           QString("%1: %2").arg(name, value.toString()),
                           Qt::darkCyan);
        }
        return;
    }

    for (const auto [idx, value] : data.asKeyValueRange()) {
        while (_samples.size() <= idx)
            _samples.append(QVector<QPointF>());

        auto tf = timestamp_ms / 1000.0;
        if (tf > _timeMax)
            _timeMax = tf;

        auto v = value.toDouble();
        auto &pts = _samples[idx];
        if (pts.size() > 0 && (tf - pts.last().x()) > 0.5) {
            //extrapolate unchanged value tail-1ms
            pts.append(QPointF(tf, pts.last().y()));
        }
        if (pts.isEmpty() && v != 0.0 && timestamp_ms != 0) {
            //extrapolate/predict initial value at start
            pts.append(QPointF(0, 0));
            pts.append(QPointF(tf, 0));
        }
        pts.append(QPointF(tf, v));
    }
}
void TelemetryFrame::rec_evt(
    quint64 timestamp_ms, QString name, QString value, QString uid, bool uplink)
{
    QColor c;
    if (name == "mission")
        c = QColor(50, 50, 100);
    else if (name == "conf")
        c = QColor(100, 100, 50);
    plot->addEvent(timestamp_ms / 1000.0, QString("%1: %2").arg(name, value), c);
}
void TelemetryFrame::rec_msg(quint64 timestamp_ms, QString text, QString subsystem) {}
void TelemetryFrame::rec_meta(quint64 timestamp_ms, QString name, QJsonObject data, bool uplink) {}
void TelemetryFrame::rec_raw(quint64 timestamp_ms, uint16_t id, QByteArray data, bool uplink) {}

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
