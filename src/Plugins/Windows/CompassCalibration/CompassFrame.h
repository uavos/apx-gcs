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
#pragma once

#include "DrawingArea.h"
#include <Fleet/Fleet.h>
#include <QWidget>

class CompassFrame : public QWidget
{
    Q_OBJECT
public:
    CompassFrame(QWidget *parent = 0);

protected:
    void closeEvent(QCloseEvent *event);

private:
    QToolBar *toolBar;
    QCheckBox checkBoxTrace;
    QPushButton buttonClear, buttonClose;
    QLabel lbInfo;
    typedef struct
    {
        QList<QColor> color;
        QList<uint> var_idx;
    } _plot;
    QMap<QAction *, _plot> map;
    DrawingArea *dArea[3];

    QVBoxLayout *vbLayout;
    QHBoxLayout *hbLayoutBottom;

    QComboBox cbSelect;

    uint tcounter;

    QList<QMetaObject::Connection> clist;

    mandala::uid_t _req_uid{};
    QTimer reqTimer;

private slots:
    void unitSelected(Unit *unit);

    void bundleData(mandala::uid_t uid, QByteArray data);

    void action_toggled(bool);
    void oncheckBoxTraceChange(int);
    void onbuttonClearPressed();

    void requestCalibrationData();

signals:
    void closed();
};
