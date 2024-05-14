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
#include "CompassFrame.h"
#include <Vehicles/Vehicles.h>
#include <QtCore>

#include <Protocols/PStream.h>

CompassFrame::CompassFrame(QWidget *parent)
    : QWidget(parent)
{
    if (this->objectName().isEmpty())
        this->setObjectName("compassFrame");
    vbLayout = new QVBoxLayout(this);
    hbLayoutBottom = new QHBoxLayout();

    buttonClear.setText(tr("Clear"));
    checkBoxTrace.setText(tr("Trace"));

    cbSelect.addItem("local");
    cbSelect.addItem("primary");
    cbSelect.addItem("secondary");
    cbSelect.addItem("failsafe");
    cbSelect.addItem("auxilary");

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(16, 16));
    toolBar->layout()->setContentsMargins(0, 0, 0, 0);
    vbLayout->addWidget(toolBar);
    buttonClose.setText(tr("Close"));
    buttonClose.setObjectName("killButton");
    //toolBar->addWidget(&buttonClose);
    //toolBar->addSeparator();
    toolBar->addWidget(&cbSelect);
    toolBar->addSeparator();
    toolBar->addWidget(&checkBoxTrace);
    toolBar->addSeparator();
    toolBar->addWidget(&buttonClear);

    vbLayout->addWidget(&lbInfo);
    vbLayout->addLayout(hbLayoutBottom);
    //vbLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum));
    /*QWidget* empty = new QWidget();
  empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  toolBar->addWidget(empty);*/

    for (int i = 0; i < 3; i++) {
        dArea[i] = new DrawingArea();
        //dArea[i]->resizeArea(this->size().width() / 3, this->size().width() / 3);
        hbLayoutBottom->insertWidget(i, dArea[i]);
    }
    dArea[0]->sAxis[0] = "X";
    dArea[0]->sAxis[1] = "Y";
    dArea[1]->sAxis[0] = "X";
    dArea[1]->sAxis[1] = "Z";
    dArea[2]->sAxis[0] = "Z";
    dArea[2]->sAxis[1] = "Y";

    reqTimer.setInterval(100);
    connect(&reqTimer, &QTimer::timeout, this, &CompassFrame::requestCalibrationData);

    connect(&checkBoxTrace, SIGNAL(stateChanged(int)), this, SLOT(oncheckBoxTraceChange(int)));
    connect(&buttonClear, SIGNAL(clicked()), this, SLOT(onbuttonClearPressed()));
    connect(&buttonClose, SIGNAL(clicked()), this, SLOT(close()));

    checkBoxTrace.setChecked(true);

    connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, &CompassFrame::vehicleSelected);
    vehicleSelected(Vehicles::instance()->current());
}
void CompassFrame::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    disconnect(this);
    emit closed();
}
void CompassFrame::vehicleSelected(Vehicle *vehicle)
{
    for (auto c : clist)
        disconnect(c);

    PVehicle *protocol = vehicle->protocol();
    if (!protocol)
        return;

    clist.append(
        connect(protocol->data(), &PData::calibrationData, this, &CompassFrame::calibrationData));
}

void CompassFrame::requestCalibrationData()
{
    PVehicle *protocol = Vehicles::instance()->current()->protocol();
    if (!protocol)
        return;
    QByteArray ba;
    ba.append((char) cbSelect.currentIndex());
    protocol->data()->requestCalibration(mandala::sns::nav::mag::uid, ba);
}

void CompassFrame::calibrationData(mandala::uid_t uid, QByteArray data)
{
    if (uid != mandala::sns::nav::mag::uid)
        return;

    PStreamReader stream(data);
    if (stream.available() != (sizeof(uint8_t) + 3 * sizeof(float)))
        return;

    uint8_t pri;
    stream >> pri;

    double x, y, z;
    x = stream.read<float>();
    y = stream.read<float>();
    z = stream.read<float>();

    dArea[0]->addData(x, y);
    dArea[1]->addData(x, z);
    dArea[2]->addData(z, y);
    lbInfo.setText(QString("bias {%1, %2, %3}\tscale {%4, %5, %6}")
                       .arg(dArea[0]->bX, 0, 'f', 2)
                       .arg(dArea[0]->bY, 0, 'f', 2)
                       .arg(dArea[1]->bY, 0, 'f', 2)
                       .arg(dArea[0]->sX, 0, 'f', 2)
                       .arg(dArea[0]->sY, 0, 'f', 2)
                       .arg(dArea[1]->sY, 0, 'f', 2));
}

void CompassFrame::action_toggled(bool checked)
{
    if (!checked)
        return;
}

void CompassFrame::oncheckBoxTraceChange(int val)
{
    if (val) {
        reqTimer.start();
        for (int i = 0; i < 3; i++) {
            dArea[i]->startTrace();
        }
    } else {
        reqTimer.stop();
        for (int i = 0; i < 3; i++) {
            dArea[i]->stopTrace();
        }
    }
}

void CompassFrame::onbuttonClearPressed()
{
    for (int i = 0; i < 3; i++) {
        dArea[i]->clearTrace();
    }
}
