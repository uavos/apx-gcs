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
#include "DatalinkStats.h"
//=============================================================================
class Datalink: public Fact
{
  Q_OBJECT

  Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged) //true when any data ever received
  Q_PROPERTY(bool online READ online WRITE setOnline NOTIFY onlineChanged) //timeout received data
  Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged) //global stream protocol errors

public:
  explicit Datalink(FactSystem *parent=0);

  static Datalink * instance() {return _instance;}

  Fact *f_readonly;

  Fact *f_listen;
  Fact *f_binded;
  Fact *f_name;
  Fact *f_pass;
  Fact *f_extctr;
  Fact *f_hbeat;

  DatalinkHosts *f_hosts;
  DatalinkPorts *f_ports;
  DatalinkClients *f_clients;

  DatalinkStats *f_stats;

private:
  static Datalink * _instance;
  QTimer heartbeatTimer; //data link alive for vehicle
  bool bReadOnly;
  QTimer onlineTimer;

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

  //counters
  void transmittedData(const QByteArray &ba);
  void receivedData(const QByteArray &ba);

  //external connections
public slots:
  void write(const QByteArray &ba);
signals:
  void read(const QByteArray &ba);
  void httpRequest(QTextStream &stream,QString req,bool *ok);

  //-----------------------------------------
  //PROPERTIES
public:
  bool valid() const;
  void setValid(const bool &v);
  bool online() const;
  void setOnline(const bool &v);
  uint errcnt() const;
  void setErrcnt(const uint &v);
private:
  bool m_valid;
  bool m_online;
  uint m_errcnt;
signals:
  void validChanged();
  void onlineChanged();
  void errcntChanged();
};
//=============================================================================
#endif

