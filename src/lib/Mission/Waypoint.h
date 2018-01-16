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
#ifndef Waypoint_H
#define Waypoint_H
//=============================================================================
#include <QtCore>
#include "MissionPathItem.h"
#include "Waypoints.h"
#include <QGeoPath>
#include <QGeoCoordinate>
//=============================================================================
class Waypoint: public MissionPathItem
{
  Q_OBJECT
  Q_ENUMS(ManeuverType)

  Q_PROPERTY(bool reachable READ reachable WRITE setReachable NOTIFY reachableChanged)
  Q_PROPERTY(bool warning READ warning WRITE setWarning NOTIFY warningChanged)

public:
  explicit Waypoint(Waypoints *parent);

  Fact *f_altitude;
  Fact *f_type;

  Fact *f_actions;
  Fact *f_speed;
  Fact *f_shot;
  Fact *f_dshot;
  Fact *f_script;
  Fact *f_poi;
  Fact *f_loiter;
  Fact *f_turnR;
  Fact *f_loops;
  Fact *f_time;


  enum ManeuverType {
    Hdg =0,
    Line,
  };
  Q_ENUM(ManeuverType)

protected:
  QGeoPath getPath();

private:
  int icourse;

private slots:
  void updateTitle();
  //void updateDescr();
  void updateActionsText();


  //---------------------------------------
  // PROPERTIES
public:
  bool reachable() const;
  void setReachable(bool v);

  bool warning() const;
  void setWarning(bool v);

protected:
  bool m_reachable;
  bool m_warning;

signals:
  void reachableChanged();
  void warningChanged();
};
//=============================================================================
#endif

