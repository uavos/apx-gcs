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
#ifndef Vehicle_H
#define Vehicle_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
#include "VehicleMandala.h"
class Vehicles;
class Nodes;
//=============================================================================
class Vehicle: public Fact
{
  Q_OBJECT
  Q_ENUMS(VehicleClass)
  Q_ENUMS(StreamType)

  Q_PROPERTY(quint16 squawk READ squawk NOTIFY squawkChanged)

public:
  enum VehicleClass {
    //must match the IDENT::_vclass type
    UAV =0,
    GCU,

    //internal use
    CURRENT,
    LOCAL
  };
  Q_ENUM(VehicleClass)

  enum StreamType {
    OFFLINE =0,
    REPLAY,
    SERVICE,
    DATA,
    XPDR,
    TELEMETRY
  };
  Q_ENUM(StreamType)



  explicit Vehicle(Vehicles *parent, QString callsign, quint16 squawk, QByteArray uid, VehicleClass vclass, bool bLocal);

  Fact * f_select; //fact action to select this vehicle
  Fact * f_squawk;
  Fact * f_uid;
  Fact * f_callsign;
  Fact * f_vclass;
  Fact * f_streamType;

  VehicleMandala *f_mandala;
  Nodes *f_nodes;

  Fact * f_selectAction;


private:
  QTimer onlineTimer;
  QTime telemetryTime;
  QTime xpdrTime;

  //data connection
public slots:
  void xpdrReceived(const QByteArray &ba);
  void downlinkReceived(const QByteArray &ba);
signals:
  void sendUplink(const QByteArray &ba);


  //---------------------------------------
  // PROPERTIES
public:
  quint16 squawk(void) const;

protected:
  quint16 m_squawk;

signals:
  void squawkChanged();


};
//=============================================================================
#endif

