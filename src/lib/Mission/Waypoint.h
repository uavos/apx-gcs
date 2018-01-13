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
#include "MissionOrderedItem.h"
#include "Waypoints.h"
#include <QGeoPath>
#include <QGeoCoordinate>
//=============================================================================
class Waypoint: public MissionOrderedItem
{
  Q_OBJECT
  Q_ENUMS(ManeuverType)

  Q_PROPERTY(QGeoPath flightPath READ flightPath NOTIFY flightPathChanged)
  Q_PROPERTY(double course READ course NOTIFY courseChanged)
  Q_PROPERTY(bool reachable READ reachable WRITE setReachable NOTIFY reachableChanged)
  Q_PROPERTY(bool warning READ warning WRITE setWarning NOTIFY warningChanged)
  Q_PROPERTY(uint time READ time NOTIFY timeChanged)
  Q_PROPERTY(uint DW READ DW NOTIFY dwChanged)
  Q_PROPERTY(uint DT READ DT NOTIFY dtChanged)
  Q_PROPERTY(uint ETA READ ETA NOTIFY etaChanged)

public:
  explicit Waypoint(Waypoints *parent);

  Fact *f_altitude;
  Fact *f_type;
  Fact *f_latitude;
  Fact *f_longitude;

  enum ManeuverType {
    Hdg =0,
    Line,
  };
  Q_ENUM(ManeuverType)

  Waypoints *waypoints;

private:
  int icourse;
  QList<QGeoCoordinate> plist;


private slots:
  void updatePath();




  //---------------------------------------
  // PROPERTIES
public:
  QGeoPath flightPath() const;

  double course() const;
  void setCourse(const double &v);

  bool reachable() const;
  void setReachable(bool v);

  bool warning() const;
  void setWarning(bool v);

  uint time() const;  //travelled time to current wp from previous [s]
  uint DW() const;    //travelled distance to current wp from previous [m]
  uint DT() const;    //estimated total travel distance [m]
  uint ETA() const;   //estimated total time of arrival [s]

protected:
  QGeoPath m_flightPath;
  double m_course;
  bool m_reachable;
  bool m_warning;
  uint m_time;
  uint m_DW;

signals:
  void flightPathChanged();
  void courseChanged();
  void reachableChanged();
  void warningChanged();
  void timeChanged();
  void dwChanged();
  void dtChanged();
  void etaChanged();
};
//=============================================================================
#endif

