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
#ifndef TelemetryPlayer_H
#define TelemetryPlayer_H
//==============================================================================
#include <QtCore>
#include <QtSql>
#include <FactSystem.h>
#include <TelemetryDB.h>
//==============================================================================
class TelemetryPlayer : public QObject
{
  Q_OBJECT
public:
  explicit TelemetryPlayer(QObject *parent = 0);


  void setTelemetryID(quint64 v);

  void setTime(quint64 v);
  quint64 time();
  bool playing();

  void setSpeed(double v);

private:
  TelemetryDB *_db;
  QSqlQuery qDownlink;

  QTimer timer;
  quint64 playTime0;
  QTime playTime;
  quint64 tNext;

  quint64 setTime0;

  quint64 m_telemetryID;
  quint64 m_time;
  bool m_playing;
  double m_speed;

private slots:
  void next();

signals:
  void timeChanged();

public slots:
  void play();
  void pause();
  void rewind();

};
//==============================================================================
#endif
