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
#ifndef PLAYER_H
#define PLAYER_H
//==============================================================================
#include <QtCore>
#include "ui_Player.h"
class VehicleRecorder;
//==============================================================================
class Player : public QDialog, public Ui::Player
{
  Q_OBJECT
public:
  explicit Player(QWidget *parent = 0);
protected:
  void closeEvent(QCloseEvent *event);
private:
  VehicleRecorder *rec;
  uint pos_ms,total_ms,play_ms;  //time in ms
  QTime  time;
  QTimer timer;
private slots:
  void mandalaFileLoaded();
  void updateStats();
  void timerStep();
  void sendData();

  void on_slider_sliderMoved(int v);
  void on_aPlay_toggled(bool checked);
  void on_aStop_triggered();
signals:
  void timeTrack(uint time_ms); //current position
  void replay_progress(uint time_ms);
  void frameUpdated(uint id);
};
//==============================================================================
#endif // PLAYER_H
