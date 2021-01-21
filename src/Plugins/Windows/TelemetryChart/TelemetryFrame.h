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
#ifndef TelemetryFrame_H
#define TelemetryFrame_H
//-----------------------------------------------------------------------------
#include "TelemetryPlot.h"
#include <Telemetry/LookupTelemetry.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryPlayer.h>
#include <Telemetry/TelemetryReader.h>
//#include <Telemetry/TelemetryShare.h>
#include <QtCore>
//=============================================================================
class TelemetryFrame : public QWidget
{
    Q_OBJECT
public:
    explicit TelemetryFrame(QWidget *parent = nullptr);

private:
    Telemetry *telemetry;

    //members of telemetry
    LookupTelemetry *lookup;
    TelemetryReader *reader;
    TelemetryPlayer *player;
    TelemetryShare *share;

    QStringList ctr_fields;

    QToolBar *toolBar;
    QVBoxLayout *vlayout;

    QAction *aFullScreen;
    QAction *aSplit;
    QAction *aShowEvents;

    QAction *aReplay;

    TelemetryPlot *plot;
    TelemetryPlot *pcopy;
    QLabel *lbTitle;

    QLineEdit *eNotes;
    QProgressBar *progressBar;
    QLabel *lbStatus;

    QToolBar *toolBarSW;
    QAction *avCLR;
    QAction *avSTD;
    QAction *avIMU;
    QAction *avCTR;

    //player
    QToolBar *toolBarPlayer;
    QSlider *playerSlider;
    QDoubleSpinBox *playerSpeed;
    QLabel *lbPlayerTime;
    QTimer plotCursorUpdateTimer;

    QDockWidget *parentW;
private slots:
    void updateStats();
    void updateData();
    void updateProgress();
    void updateStatus();

    void resetPlot();

    void eNotes_returnPressed(void);

    void aReplay_triggered(void);

    void aSplit_triggered(void);
    void aShowEvents_triggered(void);

    void avCLR_triggered(void);
    void avSTD_triggered(void);
    void avIMU_triggered(void);
    void avCTR_triggered(void);

    void playerSliderMoved();
    void plotTimeCursorMoved();
    void playerTimeChanged();
    void updatePlotPlayerTime();
};
//=============================================================================
#endif
