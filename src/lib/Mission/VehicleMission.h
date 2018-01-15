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
#include <QGeoCoordinate>
#include "FactSystem.h"
class Vehicle;
class MissionItems;
class Waypoints;
class Runways;
class Taxiways;
class Points;
//=============================================================================
class VehicleMission: public Fact
{
  Q_OBJECT
  Q_PROPERTY(QGeoCoordinate startPoint READ startPoint WRITE setStartPoint NOTIFY startPointChanged)
  Q_PROPERTY(double startHeading READ startHeading WRITE setStartHeading NOTIFY startHeadingChanged)
  Q_PROPERTY(double startLength READ startLength WRITE setStartLength NOTIFY startLengthChanged)

public:
  explicit VehicleMission(Vehicle *parent);

  Fact *f_request;
  Fact *f_reload;
  Fact *f_upload;
  Fact *f_stop;

  Waypoints   *f_waypoints;
  Runways     *f_runways;
  Taxiways    *f_taxiways;
  Points      *f_points;
  MissionItems *f_restricted;
  MissionItems *f_emergency;

  Vehicle *vehicle;


  bool unpackMission(const QByteArray &ba);

private:

private slots:
  void updateActions();
  void updateStartPath();

public slots:
  void clear();
  void upload();
  void stop();

signals:
  void actionsUpdated();


  //---------------------------------------
  // PROPERTIES
public:
  QGeoCoordinate startPoint() const;
  void setStartPoint(const QGeoCoordinate &v);

  double startHeading() const;
  void setStartHeading(const double &v);

  double startLength() const;
  void setStartLength(const double &v);

protected:
  QGeoCoordinate m_startPoint;
  double m_startHeading;
  double m_startLength;

signals:
  void startPointChanged();
  void startHeadingChanged();
  void startLengthChanged();
};
//=============================================================================
#endif

