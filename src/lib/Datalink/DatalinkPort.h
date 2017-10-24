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
#ifndef DatalinkPort_H
#define DatalinkPort_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
class DatalinkPorts;
class DatalinkHost;
class Serial;
//=============================================================================
class DatalinkPort: public Fact
{
  Q_OBJECT
public:
  explicit DatalinkPort(DatalinkPorts *parent,const DatalinkPort *port=NULL);

  DatalinkPorts *f_ports;

  Fact *f_enabled;
  Fact *f_type;
  Fact *f_dev;
  Fact *f_baud;
  Fact *f_host;

  Fact *f_local;

  Fact *f_save;
  Fact *f_remove;

  DatalinkHost *if_host;
  Serial *if_serial;

  bool active();

private:
  bool _new;

private slots:
  void updateStats();
  void enable();
  void disable();
  void enabledChanged();
  void syncDevEnum();
  void syncHostEnum();
public slots:
  void defaults();

  //iface connect
private slots:
  void ifacePacketReceived(const QByteArray &ba);
  void disconnectAll();
  void serialConnected(QString pname);
  void serialDisconnected();
  void hostStatusChanged();

public slots:
  void connectPort();

  //data
signals:
  void packetReceived(const QByteArray &ba);
public slots:
  void sendPacket(const QByteArray &ba);

};
//=============================================================================
#endif

