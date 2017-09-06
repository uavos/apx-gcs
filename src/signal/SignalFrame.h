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
#ifndef SIGNALFRAME_H
#define SIGNALFRAME_H
//==============================================================================
#include <QtWidgets>
#include "QMandala.h"
#include "ui_SignalFrame.h"
//==============================================================================
class SignalFrame : public QWidget, public Ui::SignalFrame
{
  Q_OBJECT
public:
  SignalFrame(QWidget *parent = 0);
private:
  QToolBar *toolBar;
  QActionGroup *actionGroup;
  QAction *curAction;
  typedef struct {
    QList<QColor> color;
    QList<uint> var_idx;
  }_plot;
  QMap<QAction*,_plot>map;
  QAction* addPlot(QString name,const QList<QColor> &color,const QList<uint> &var_idx);
  QList<QMetaObject::Connection>mcon;
private slots:
  void mandalaCurrentChanged(QMandalaItem *m);
  void dataReceived(uint var_idx);
  void action_toggled(bool);
};
//==============================================================================
#endif // SIGNALFRAME_H
