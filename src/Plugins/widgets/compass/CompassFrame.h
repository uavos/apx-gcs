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
#ifndef COMPASSFRAME_H
#define COMPASSFRAME_H
//==============================================================================
#include "DrawingArea.h"
#include <Vehicles/VehicleMandalaValue.h>
#include <QWidget>
//==============================================================================
class CompassFrame : public QWidget
{
    Q_OBJECT
public:
    CompassFrame(QWidget *parent = 0);

protected:
    void closeEvent(QCloseEvent *event);

private:
    CurrentVehicleMandalaValue<double> c_Hx;
    CurrentVehicleMandalaValue<double> c_Hy;
    CurrentVehicleMandalaValue<double> c_Hz;

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

    uint tcounter;
private slots:
    void dataReceived();
    void action_toggled(bool);
    void oncheckBoxTraceChange(int);
    void onbuttonClearPressed();
signals:
    void closed();
};
//==============================================================================
#endif // CompassFrame_H
