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
#ifndef VehicleMission_H
#define VehicleMission_H
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

  Q_PROPERTY(QGeoCoordinate mapCoordinate READ mapCoordinate WRITE setMapCoordinate NOTIFY mapCoordinateChanged)

  Q_PROPERTY(QGeoCoordinate startPoint READ startPoint WRITE setStartPoint NOTIFY startPointChanged)
  Q_PROPERTY(double startHeading READ startHeading WRITE setStartHeading NOTIFY startHeadingChanged)
  Q_PROPERTY(double startLength READ startLength WRITE setStartLength NOTIFY startLengthChanged)

  Q_PROPERTY(MissionListModel * listModel READ listModel CONSTANT)

  Q_PROPERTY(int missionSize READ missionSize NOTIFY missionSizeChanged)

public:
  explicit VehicleMission(Vehicle *parent);

  enum MissionItemType {
    WaypointType =0,
    RunwayType,
    TaxiwayType,
    PoiType,
  };

  Q_ENUM(MissionItemType)

  typedef MissionGroupT<Runway,RunwayType>      Runways;
  typedef MissionGroupT<Waypoint,WaypointType>  Waypoints;
  typedef MissionGroupT<Taxiway,TaxiwayType>    Taxiways;
  typedef MissionGroupT<Poi,PoiType>            Pois;

  Runways   *f_runways;
  Waypoints *f_waypoints;
  Taxiways  *f_taxiways;
  Pois      *f_pois;
  //MissionItems *f_restricted;
  //MissionItems *f_emergency;

  Fact *f_missionTitle;

  FactAction *f_request;
  FactAction *f_clear;
  FactAction *f_upload;

  Fact *f_tools;
  Fact *f_storage;

  Vehicle *vehicle;

  QList<MissionGroup*> groups;

  bool unpackMission(const QByteArray &ba);



  Q_INVOKABLE QGeoRectangle boundingGeoRectangle() const;


  //Fact override
  void backup();
  void restore();

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
  void missionReceived();
  void actionsUpdated();


  //---------------------------------------
  // PROPERTIES
public:
  QGeoCoordinate mapCoordinate() const;
  void setMapCoordinate(const QGeoCoordinate &v);

  QGeoCoordinate startPoint() const;
  void setStartPoint(const QGeoCoordinate &v);

  double startHeading() const;
  void setStartHeading(const double &v);

  double startLength() const;
  void setStartLength(const double &v);

  MissionListModel * listModel() const;

  int missionSize() const;
  void setMissionSize(const int v);

protected:
  QGeoCoordinate m_mapCoordinate;
  QGeoCoordinate m_startPoint;
  double m_startHeading;
  double m_startLength;

  MissionListModel *m_listModel;

  int m_missionSize;

signals:
  void mapCoordinateChanged();
  void startPointChanged();
  void startHeadingChanged();
  void startLengthChanged();
  void missionSizeChanged();
};
//=============================================================================
#endif

