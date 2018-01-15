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
#ifndef MissionPathItem_H
#define MissionPathItem_H
//=============================================================================
#include <QtCore>
#include "MissionOrderedItem.h"
#include "MissionPathItems.h"
#include <QGeoPath>
#include <QGeoCoordinate>
//=============================================================================
class MissionPathItem: public MissionOrderedItem
{
  Q_OBJECT
  Q_PROPERTY(QGeoPath travelPath READ travelPath NOTIFY travelPathChanged)
  Q_PROPERTY(double course READ course NOTIFY courseChanged)
  Q_PROPERTY(uint time READ time NOTIFY timeChanged)
  Q_PROPERTY(uint distance READ distance NOTIFY distanceChanged)

  Q_PROPERTY(uint totalDistance READ totalDistance NOTIFY totalDistanceChanged)
  Q_PROPERTY(uint totalTime READ totalTime NOTIFY totalTimeChanged)

public:
  explicit MissionPathItem(MissionPathItems *parent, const QString &name, const QString &title, const QString &descr);

  Fact *f_latitude;
  Fact *f_longitude;

  MissionPathItems *pathItems;

private:

protected:
  virtual QGeoPath getPath();

private slots:
  virtual void updateStatus();

public slots:
  void updatePath();
  void resetPath();

  //---------------------------------------
  // PROPERTIES
public:
  QGeoPath travelPath() const;

  double course() const;
  void setCourse(const double &v);

  uint time() const;  //travelled time to current wp from previous [s]
  void setTime(uint v);

  uint distance() const;    //travelled distance to current wp from previous [m]
  void setDistance(uint v);

  uint totalDistance() const;    //estimated total travel distance [m]
  void setTotalDistance(uint v);

  uint totalTime() const;   //estimated total time of arrival [s]
  void setTotalTime(uint v);

protected:
  QGeoPath m_travelPath;
  double m_course;
  uint m_time;
  uint m_distance;

  uint m_totalDistance;
  uint m_totalTime;

signals:
  void travelPathChanged();
  void courseChanged();
  void timeChanged();
  void distanceChanged();

  void totalDistanceChanged();
  void totalTimeChanged();
};
//=============================================================================
#endif

