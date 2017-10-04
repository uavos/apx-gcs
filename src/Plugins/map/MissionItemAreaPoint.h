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
#ifndef MissionItemAreaPoint_H
#define MissionItemAreaPoint_H
#include "MissionItemArea.h"
class ItemBase;
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemAreaPoint: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemAreaPoint(MissionItemArea *parent,QString name);
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;
  QStringList getToolTip(void) const;

  //fields
  MissionItemField *f_latitude;
  MissionItemField *f_longitude;

  MissionItemAreaPoint *prevItem() const;
  MissionItemAreaPoint *nextItem() const;
  //path est sim
  double distance;//travelled distance to current wp from previous
  QList<QPointF> path; //saved path (lat,lon)

  double DME() const; //estimated travel distance (total)

protected:
  void remove(void);
private:
  QColor color;

signals:
  void pathChanged();
};
//=============================================================================
#endif
