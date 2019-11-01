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
#include "CompassFrame.h"
#include <Vehicles/Vehicles.h>
#include <QtCore>
//==============================================================================
CompassFrame::CompassFrame(QWidget *parent)
    : QWidget(parent)
    , c_Hx("Hx")
    , c_Hy("Hy")
    , c_Hz("Hz")
{
    if (this->objectName().isEmpty())
        this->setObjectName("compassFrame");
    vbLayout = new QVBoxLayout(this);
    hbLayoutBottom = new QHBoxLayout();

    buttonClear.setText(tr("Clear"));
    checkBoxTrace.setText(tr("Trace"));

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(16, 16));
    toolBar->layout()->setMargin(0);
    vbLayout->addWidget(toolBar);
    buttonClose.setText(tr("Close"));
    buttonClose.setObjectName("killButton");
    //toolBar->addWidget(&buttonClose);
    //toolBar->addSeparator();
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

    connect(&checkBoxTrace, SIGNAL(stateChanged(int)), this, SLOT(oncheckBoxTraceChange(int)));
    connect(&buttonClear, SIGNAL(clicked()), this, SLOT(onbuttonClearPressed()));
    connect(&buttonClose, SIGNAL(clicked()), this, SLOT(close()));

    checkBoxTrace.setChecked(true);

    connect(Vehicles::instance(),
            &Vehicles::currentDownstreamDataReceived,
            this,
            &CompassFrame::dataReceived);
}
void CompassFrame::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    disconnect(this);
    emit closed();
}
//=============================================================================
void CompassFrame::dataReceived()
{
    dArea[0]->addData(c_Hx, c_Hy);
    dArea[1]->addData(c_Hx, c_Hz);
    dArea[2]->addData(c_Hz, c_Hy);
    lbInfo.setText(QString("bias {%1, %2, %3}\tscale {%4, %5, %6}")
                       .arg(dArea[0]->bX, 0, 'f', 2)
                       .arg(dArea[0]->bY, 0, 'f', 2)
                       .arg(dArea[1]->bY, 0, 'f', 2)
                       .arg(dArea[0]->sX, 0, 'f', 2)
                       .arg(dArea[0]->sY, 0, 'f', 2)
                       .arg(dArea[1]->sY, 0, 'f', 2));
}
//==============================================================================
void CompassFrame::action_toggled(bool checked)
{
    if (!checked)
        return;
}
//==============================================================================
void CompassFrame::oncheckBoxTraceChange(int val)
{
    if (val) {
        for (int i = 0; i < 3; i++) {
            dArea[i]->startTrace();
        }
    } else {
        for (int i = 0; i < 3; i++) {
            dArea[i]->stopTrace();
        }
    }
}
//==============================================================================
void CompassFrame::onbuttonClearPressed()
{
    for (int i = 0; i < 3; i++) {
        dArea[i]->clearTrace();
    }
}
