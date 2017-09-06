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
#ifndef Joystick_H
#define Joystick_H
//=============================================================================
#include <QtCore>
#include <QSocketNotifier>
//=============================================================================
class Joystick : public QObject
{
  Q_OBJECT
public:
  explicit Joystick(QObject *parent = 0);
private:
  typedef struct {
    QMap<uint,QString> buttons;
    QMap<uint,QString> axes;
    QString fname;
    QSocketNotifier *socketNotifier;
    QString name;
    int version,axes_cnt,buttons_cnt;
    uint ax_init;      //protect initial axis values
  }_jsw;
  QMap<int,_jsw> jsw; //joysticks by fd
  QStringList jsw_list;
  QTimer timer_open;
  QSettings *cfg;

  //sliders timer rate limit
  QHash<QString,QString> scrPending;
  QTimer scrTimer;



  bool open(const QString &fname,const QString &group,const QString &filter);
  void close(int fd);
private slots:
  void readEvent(int fd);
  void scan();
  //sliders timer rate limit
  void scrTimerTimeout();
};
//=============================================================================
#endif // Joystick_H
