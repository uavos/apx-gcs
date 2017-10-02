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
#ifndef MissionItemTw_H
#define MissionItemTw_H
#include "MissionItemObject.h"
#include "MissionPath.h"
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemTw: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemTw(MissionItemCategory<MissionItemTw> *parent);
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;
  QStringList getToolTip(void) const;

  //fields
  MissionItemField *f_latitude;
  MissionItemField *f_longitude;

  MissionItemTw *prevItem() const;
  MissionItemTw *nextItem() const;
protected:
  QByteArray pack() const;
  int unpack(const QByteArray &ba);

private slots:
  void updatePath();

  //PROPERTIES
public:
  Q_PROPERTY(MissionPath * path READ path NOTIFY pathChanged)
  MissionPath * path();
  Q_PROPERTY(int distance READ distance NOTIFY distanceChanged)
  int distance() const; //travelled distance to current wp from previous [m]
private:
  MissionPath m_path;
  int m_distance;//travelled distance to current wp from previous
signals:
  void pathChanged();
  void distanceChanged();
};
//=============================================================================
#endif
