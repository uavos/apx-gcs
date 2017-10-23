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
#ifndef Datalink_H
#define Datalink_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
#include "DatalinkHosts.h"
#include "DatalinkPorts.h"
#include "DatalinkClients.h"
//=============================================================================
class Datalink: public Fact
{
  Q_OBJECT
public:
  explicit Datalink(FactSystem *parent=0);

  Fact *f_readonly;

  Fact *f_active;
  Fact *f_binded;
  Fact *f_name;
  Fact *f_pass;
  Fact *f_extctr;
  Fact *f_hbeat;

  DatalinkHosts *f_hosts;
  DatalinkPorts *f_ports;
  DatalinkClients *f_clients;

  Fact *f_stats;
  Fact *f_upcnt;
  Fact *f_dncnt;
  Fact *f_uprate;
  Fact *f_dnrate;

private:
  QTimer heartbeatTimer; //data link alive for vehicle

private slots:
  void readonlyChanged();
  void heartbeatTimeout();
  void hbeatChanged();


  //internal connections
public slots:
  void packetReceivedFromClient(const QByteArray &ba);
  void packetReceivedFromHost(const QByteArray &ba);
  void packetReceivedFromPort(const QByteArray &ba);
signals:
  void sendPacketToClients(const QByteArray &ba);
  void sendPacketToHosts(const QByteArray &ba);
  void sendPacketToPorts(const QByteArray &ba);

  //external connections
public slots:
  void write(const QByteArray &ba);
signals:
  void read(const QByteArray &ba);

};
//=============================================================================
#endif

