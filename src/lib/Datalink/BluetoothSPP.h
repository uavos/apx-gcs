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
#ifndef BluetoothSPP_H
#define BluetoothSPP_H
#include <inttypes.h>
#include <QtCore>
#include <QBasicTimer>
#include <QtEvents>
#include <QSettings>
#include <QDir>
#include <QDate>
#include <QSocketNotifier>
//-----------------------------------------------------------------------------
#include "comm.h"
//=============================================================================
class tryOpenThread : public QThread
{
  Q_OBJECT
public:
  tryOpenThread(QObject *parent,Comm *uart,QString name,uint baudrate)
      :QThread(parent),result(false),uart(uart),name(name),baudrate(baudrate){}
  bool result;
protected:
  Comm *uart;
  QString name;
  uint baudrate;
  void run(){
    result=uart->open(name.toUtf8().data(),baudrate);
  }
};
//-----------------------------------------------------------------------------
class BluetoothSPP : public QObject
{
  Q_OBJECT
public:
  BluetoothSPP(int num=-1,QObject * parent = 0,bool active=true);
  bool isOpen(void);

protected:
private:
  int num;
  Comm uart;
  QString pname;
  uint scanIdx;

  QTime timeout;

  QSocketNotifier *socketNotifier;

  uint8_t rxbuf[8192];

  void tryOpen(QString name,uint baudrate);
  tryOpenThread *thr;

private slots:

  //ports
  bool open();
  void read();

  void openFinished();

public slots:
  void activate();
  void close();
  void send(const QByteArray &ba);
signals:
  void received(const QByteArray &ba);
};
//=============================================================================
#endif
