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
#ifndef Mission_H
#define Mission_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
class Vehicle;
class MissionItems;
class Waypoints;
//=============================================================================
class VehicleMission: public Fact
{
  Q_OBJECT

public:
  explicit VehicleMission(Vehicle *parent);

  Fact *f_request;
  Fact *f_reload;
  Fact *f_upload;
  Fact *f_stop;

  Waypoints *f_waypoints;
  MissionItems *f_runways;
  MissionItems *f_taxiways;
  MissionItems *f_points;
  MissionItems *f_restricted;
  MissionItems *f_emergency;

  Vehicle *vehicle;


  bool unpackMission(const QByteArray &ba);

private:

private slots:
  void updateActions();

public slots:
  void clear();
  void upload();
  void stop();

signals:
  void actionsUpdated();


  //---------------------------------------
  // PROPERTIES
public:

protected:

signals:
};
//=============================================================================
#endif

