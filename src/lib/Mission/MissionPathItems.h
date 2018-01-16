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
#ifndef MissionPathItems_H
#define MissionPathItems_H
//=============================================================================
#include <QtCore>
#include "MissionItems.h"
#include <QGeoPath>
#include <QGeoCoordinate>
class VehicleMission;
//=============================================================================
class MissionPathItems: public MissionItems
{
  Q_OBJECT
  Q_PROPERTY(uint distance READ distance NOTIFY distanceChanged)
  Q_PROPERTY(uint time READ time NOTIFY timeChanged)

public:
  explicit MissionPathItems(VehicleMission *parent, VehicleMission::MissionItemType itemType, const QString &name, const QString &title, const QString &descr);


private:

private slots:
  virtual void updateDescr();

public slots:
  void updateTime();
  void updateDistance();




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
#endif

