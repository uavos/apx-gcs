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
#ifndef DatalinkHost_H
#define DatalinkHost_H
//=============================================================================
#include <QtCore>
#include "DatalinkSocket.h"
class DatalinkHosts;
//=============================================================================
class DatalinkHost: public DatalinkSocket
{
  Q_OBJECT
public:
  explicit DatalinkHost(DatalinkHosts *parent, QString title, QHostAddress host);

  QHostAddress host;

  bool connectionActive() const;

private:
  DatalinkHosts *container;
  QTime time;
  QTimer updateStatsTimer;
  QTimer reconnectTimer;

  bool bReconnect;
  uint retry;

private slots:
  void updateStats();

  void socketDisconnected();
  void reconnect();

public slots:
  void updateTimeout();
  void connectToServer();
  void disconnectAll();
};
//=============================================================================
#endif

