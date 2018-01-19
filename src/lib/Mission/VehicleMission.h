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
#include <QGeoRectangle>
#include <FactSystem.h>
#include "MissionGroup.h"
class Vehicle;
class MissionListModel;
class Waypoint;
class Runway;
class Taxiway;
class Poi;
//=============================================================================
class VehicleMission: public Fact
{
  Q_OBJECT
  Q_ENUMS(MissionItemType)

  Q_PROPERTY(QGeoCoordinate startPoint READ startPoint WRITE setStartPoint NOTIFY startPointChanged)
  Q_PROPERTY(double startHeading READ startHeading WRITE setStartHeading NOTIFY startHeadingChanged)
  Q_PROPERTY(double startLength READ startLength WRITE setStartLength NOTIFY startLengthChanged)

  Q_PROPERTY(MissionListModel * listModel READ listModel CONSTANT)

  Q_PROPERTY(bool empty READ empty NOTIFY emptyChanged)
  Q_PROPERTY(int missionSize READ missionSize NOTIFY missionSizeChanged)

public:
  explicit VehicleMission(Vehicle *parent);

  Fact *f_request;
  Fact *f_clear;
  Fact *f_upload;

  MissionGroup *f_waypoints;
  MissionGroup *f_runways;
  MissionGroup *f_taxiways;
  MissionGroup *f_pois;
  //MissionItems *f_restricted;
  //MissionItems *f_emergency;

  Vehicle *vehicle;

  QList<MissionGroup*> groups;

  bool unpackMission(const QByteArray &ba);


  enum MissionItemType {
    WaypointType =0,
    RunwayType,
    TaxiwayType,
    PoiType,
  };
  Q_ENUM(MissionItemType)

  Q_INVOKABLE QGeoRectangle boundingGeoRectangle() const;

private:

private slots:
  void updateSize();
  void updateActions();
  void updateStartPath();

public slots:
  void clearMission();
  void uploadMission();
  void test(int n=50);

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

  MissionListModel * listModel() const;

  bool empty() const;
  void setEmpty(const bool v);

  int missionSize() const;
  void setMissionSize(const int v);

protected:
  QGeoCoordinate m_startPoint;
  double m_startHeading;
  double m_startLength;

  MissionListModel *m_listModel;

  bool m_empty;
  int m_missionSize;

signals:
  void startPointChanged();
  void startHeadingChanged();
  void startLengthChanged();
  void emptyChanged();
  void missionSizeChanged();
};
//=============================================================================
#endif

