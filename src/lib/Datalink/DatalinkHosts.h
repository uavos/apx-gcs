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
#ifndef DatalinkHosts_H
#define DatalinkHosts_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
class Datalink;
class DatalinkHost;
//=============================================================================
class DatalinkHosts: public Fact
{
  Q_OBJECT

  Q_PROPERTY(int connectedCount READ connectedCount NOTIFY connectedCountChanged)
  Q_PROPERTY(int availableCount READ availableCount NOTIFY availableCountChanged)

public:
  explicit DatalinkHosts(Datalink *parent);

  Fact *f_add;
  Fact *f_host;
  FactAction *f_connect;


  FactAction *f_alloff;
  Fact *f_list;

  Datalink *f_datalink;

  DatalinkHost *f_localhost;

  DatalinkHost *registerHost(QHostAddress addr, QString sname,bool bPort=false);
  DatalinkHost *hostByAddr(QHostAddress addr);

private:
  QUdpSocket *udpReader;
  QUdpSocket *udpAnnounce;

  QTimer announceTimer;

private slots:
  //UDP discover service
  void announce(void);
  void tryBind(void);
  void udpRead(void);

  void serverBindedChanged();

  void connectTriggered();

public slots:
  bool connectToServer(QHostAddress haddr);
  void updateStats();
  void updateConnectedStatus();
  void connectLocalhost();


  //-----------------------------------------
  //PROPERTIES
public:
  virtual int connectedCount() const;
  virtual int availableCount() const;
protected:
  int m_connectedCount;
signals:
  void connectedCountChanged();
  void availableCountChanged();
};
//=============================================================================
#endif

