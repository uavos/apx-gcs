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
#ifndef MissionItemPi_H
#define MissionItemPi_H
#include "MissionItemObject.h"
#include "MissionPath.h"
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemPi: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemPi(MissionItemCategory<MissionItemPi> *parent);
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;
  QStringList getToolTip(void) const;

  //fields
  MissionItemField *f_latitude;
  MissionItemField *f_longitude;
  MissionItemField *f_HMSL;
  MissionItemField *f_turnR;
  MissionItemField *f_loops;
  MissionItemField *f_time;
protected:
  QByteArray pack() const;
  int unpack(const QByteArray &ba);


  //PROPERTIES
public:
  Q_PROPERTY(QPointF turnPoint READ turnPoint WRITE setTurnPoint NOTIFY turnPointChanged)
  QPointF turnPoint();
  Q_PROPERTY(MissionPath * pathTurn READ pathTurn NOTIFY turnPointChanged)
  MissionPath * pathTurn();
  void setTurnPoint(QPointF ll);
private:
  MissionPath m_pathTurn;
signals:
  void turnPointChanged();
};
//=============================================================================
#endif
