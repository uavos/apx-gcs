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
#ifndef DatalinkFacts_H
#define DatalinkFacts_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
//=============================================================================
class DatalinkFacts: public Fact
{
  Q_OBJECT
public:
  explicit DatalinkFacts(FactSystem *parent=0);

  Fact *f_readonly;

  Fact *f_active;
  Fact *f_binded;
  Fact *f_name;
  Fact *f_pass;
  Fact *f_extctr;
  Fact *f_hbeat;

  Fact *f_hosts;
  Fact *f_ports;

  Fact *f_clients;

  Q_INVOKABLE bool isLocalHost(const QHostAddress address) const;

private slots:
  void readonlyChanged();
  void activeChanged();
};
//=============================================================================
#endif

