﻿/*
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
#ifndef Vehicles_H
#define Vehicles_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
#include "Vehicle.h"
//=============================================================================
class Vehicles: public Fact
{
  Q_OBJECT

  Q_PROPERTY(Vehicle * current READ current NOTIFY currentChanged)

public:
  explicit Vehicles(FactSystem *parent);

  static Vehicles * instance() {return _instance;}

  Fact *f_list;

  Fact *f_select;

  Vehicle *f_local;

private:
  static Vehicles * _instance;

  //IDENT procedures
  QTimer reqTimer;
  QList<QByteArray> reqList;
  void reqIDENT(quint16 squawk);
  void assignIDENT(QString callsign, QByteArray uid);
  void scheduleRequest(const QByteArray &ba);

  //ident lookup
  QMap<quint16,Vehicle*> squawkMap;

public slots:
  void selectVehicle(Vehicle *v);
signals:
  void vehicleRegistered(Vehicle*);
  void vehicleRemoved(Vehicle*);
  void vehicleSelected(Vehicle*);

  //data connection
public slots:
  void downlinkReceived(const QByteArray &ba);
  void vehicleSendUplink(Vehicle *v,const QByteArray &packet);
signals:
  void sendUplink(const QByteArray &packet);

  //---------------------------------------
  // PROPERTIES
public:
  Vehicle * current(void) const;

protected:
  Vehicle * m_current;

signals:
  void currentChanged();


};
//=============================================================================
#endif
