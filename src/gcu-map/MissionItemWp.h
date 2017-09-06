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
#include "MissionPath.h"
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemWp: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemWp(MissionItemCategory<MissionItemWp> *parent=0);
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

  MissionItemWp *prevItem() const;
  MissionItemWp *nextItem() const;

protected:
  QByteArray pack() const;
  int unpack(const QByteArray &ba);
private slots:
  void updatePath();
  void updateStartPath();
private:
  int icourse;



  //PROPERTIES
public:
  Q_PROPERTY(MissionPath * path READ path NOTIFY pathChanged)
  Q_PROPERTY(bool reachable READ reachable WRITE setReachable NOTIFY reachableChanged)
  Q_PROPERTY(bool warning READ warning WRITE setWarning NOTIFY warningChanged)
  Q_PROPERTY(double course READ course NOTIFY courseChanged)
  Q_PROPERTY(uint time READ time NOTIFY timeChanged)
  Q_PROPERTY(uint DW READ DW NOTIFY dwChanged)
  Q_PROPERTY(uint DT READ DT NOTIFY dtChanged)
  Q_PROPERTY(uint ETA READ ETA NOTIFY etaChanged)

  MissionPath * path();

  bool reachable();
  void setReachable(bool v);
  bool warning();
  void setWarning(bool v);
  double course();  //which crs passed wpt
  uint time() const;  //travelled time to current wp from previous [s]
  uint DW() const; //travelled distance to current wp from previous [m]
  uint DT() const; //estimated total travel distance [m]
  uint ETA() const; //estimated total time of arrival [s]
private:
  MissionPath m_path;
  bool m_reachable;
  bool m_warning;
  double m_course;
  uint m_time;    //travelled time to current wp from previous
  uint m_DW;//travelled distance to current wp from previous
signals:
  void pathChanged();
  void reachableChanged();
  void warningChanged();
  void courseChanged();
  void timeChanged();
  void dwChanged();
  void dtChanged();
  void etaChanged();
};
//=============================================================================
#endif
