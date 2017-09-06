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
#ifndef JOYSTICKFRAME_H
#define JOYSTICKFRAME_H
//==============================================================================
#include <QtWidgets>
#include "ui_JoystickFrame.h"
class FlowLayout;
//==============================================================================
class JoystickFrame : public QWidget, public Ui::JoystickFrame
{
  Q_OBJECT
public:
  JoystickFrame(QWidget *parent = 0);
private:
  QSettings *conf;
  QToolBar *toolBar;
  FlowLayout *btnLayout;

  QHash<QWidget*,QString> ctrMap;

  QString caption(QString cmd);

  QHash<QString,QString> scrPending;
  QTimer scrTimer;

private slots:
  void clear();
  void load(QString group);
  void on_aConfig_triggered();
  void on_aReload_triggered();
  void on_cbJsw_currentIndexChanged(int index);
  //controls
  void buttonClicked();
  void sliderMoved(int v);
  void XChanged(double x);
  void YChanged(double y);
  //sliders timer rate limit
  void scrTimerTimeout();
};
//==============================================================================
#endif // JOYSTICKFRAME_H
