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
#ifndef Serial_H
#define Serial_H
#include <inttypes.h>
#include <QtCore>
#include <QSettings>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <escaped.h>
//-----------------------------------------------------------------------------
class SerialWorker;
//=============================================================================
class Serial : public QObject
{
  Q_OBJECT
public:
  Serial(int num=-1,QObject * parent = 0,bool active=true);
  ~Serial();

private:
  QThread thr;
  SerialWorker *worker;
signals:
  void w_activate();
  void w_close();
  void w_send(const QByteArray ba);
private slots:
  void w_received(QByteArray ba);


public slots:
  void activate();
  void close();
  void send(const QByteArray ba);
signals:
  void received(QByteArray ba);
};
//=============================================================================
class SerialWorker : public QSerialPort, public _escaped
{
  Q_OBJECT
public:
  SerialWorker(int num=-1);
  ~SerialWorker();

private:
  int num;
  QSerialPort *sp;
  QSocketNotifier *socketNotifier;

  //options
  QString o_pname;
  uint o_brate;

  QTimer *openTimer;
  QSettings settings;

  bool isOpen();
  bool m_open;

  QByteArray txba;

  QSerialPortInfo portInfo;
  int fd;


  //[auto] ports scanner
  bool isAvailable(const QSerialPortInfo &spi);
  static QStringList openPorts;
  int scan_idx;

  void errClose();

  bool openPort(const QSerialPortInfo &spi,int baudrate);

  unsigned int getRxCnt(void);

protected:
  //esc
  uint esc_read(uint8_t *buf,uint sz);
  bool esc_write_byte(const uint8_t v);
  void escWriteDone(void);

private slots:
  void tryOpen();

  void serialError(QSerialPort::SerialPortError error);

  void newDataAvailable();

public slots:
  void activate();
  void closePort();
  void send(const QByteArray ba);
signals:
  void received(QByteArray ba);
  void startOpenWorker();
};
//=============================================================================
#endif
