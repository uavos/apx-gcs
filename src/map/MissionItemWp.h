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
#ifndef MissionItemWp_H
#define MissionItemWp_H
#include "MissionItemObject.h"
class ItemBase;
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemWp: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemWp(MissionItemCategory<MissionItemWp> *parent);
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;
  QStringList getToolTip(void) const;

  //fields
  MissionItemField *f_altitude;
  MissionItemField *f_type;
  MissionItemField *f_latitude;
  MissionItemField *f_longitude;

  MissionItem      *f_actions;
  MissionItemField *f_speed;
  MissionItemField *f_poi;
  MissionItemField *f_loiter;
  MissionItemField *f_turnR;
  MissionItemField *f_loops;
  MissionItemField *f_time;
  MissionItemField *f_shot;
  MissionItemField *f_scr;
  MissionItemField *f_dshot;

  MissionItemWp *prevItem() const;
  MissionItemWp *nextItem() const;
  //path est sim
  double course;  //which crs passed wpt
  double distance;//travelled distance to current wp from previous
  double time;    //travelled time to current wp from previous
  QList<QPointF> path; //saved path (lat,lon)
  bool wptReached,wptWarning;

  double ETA() const; //estimated time of arrival (total)
  double DME() const; //estimated travel distance (total)
protected:
  QByteArray pack() const;
  int unpack(const QByteArray &ba);
private slots:
  void updatePath();
signals:
  void pathChanged();
};
//=============================================================================
#endif
