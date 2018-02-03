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
#ifndef MissionGroup_H
#define MissionGroup_H
//=============================================================================
#include <QtCore>
#include <FactSystem.h>
#include <QGeoCoordinate>
class VehicleMission;
class MissionItem;
//=============================================================================
class MissionGroup: public Fact
{
  Q_OBJECT
  Q_PROPERTY(uint distance READ distance NOTIFY distanceChanged)
  Q_PROPERTY(uint time READ time NOTIFY timeChanged)

public:
  explicit MissionGroup(VehicleMission *parent, const QString &name, const QString &title, const QString &descr);

  VehicleMission *mission;

  virtual int missionItemType() const {return -1;}

  virtual MissionItem * createObject() {return NULL;}

  MissionItem * addObject(const QGeoCoordinate &);


  FactAction *f_clear;

private:
  QTimer updateTimeTimer;
  QTimer updateDistanceTimer;
  QString _descr;

protected:
  void objectAdded(Fact *fact);

private slots:
  void updateTimeDo();
  void updateDistanceDo();
  void updateDescr();
  void updateStatus();

  void clearGroup();

public slots:
  void updateTime();
  void updateDistance();

  void add();

  //---------------------------------------
  // PROPERTIES
public:
  uint distance() const; //estimated total travel distance [m]
  void setDistance(uint v);

  uint time() const;    //estimated total travel time [sec]
  void setTime(uint v);

protected:
  uint m_distance;
  uint m_time;

signals:
  void distanceChanged();
  void timeChanged();
};
//=============================================================================
template <class T, int miType>
class MissionGroupT: public MissionGroup
{
public:
  explicit MissionGroupT(VehicleMission *parent, const QString &name, const QString &title, const QString &descr)
    : MissionGroup(parent,name,title,descr)
  {
  }
  int missionItemType() const
  {
    return miType;
  }

  MissionItem * createObject()
  {
    return new T(this);
  }
};
//=============================================================================
#endif

