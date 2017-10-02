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
#ifndef MissionItemRw_H
#define MissionItemRw_H
#include "MissionItemObject.h"
#include "MissionPath.h"
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemRw: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemRw(MissionItemCategory<MissionItemRw> *parent);

  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;

  //fields
  MissionItemField *f_turn;
  MissionItemField *f_approach;
  MissionItemField *f_HMSL;
  MissionItemField *f_latitude;
  MissionItemField *f_longitude;
  MissionItemField *f_dN;
  MissionItemField *f_dE;
protected:
  QByteArray pack() const;
  int unpack(const QByteArray &ba);

private slots:
  void updatePath();
  void updateHeading();
  void updateStartPoint();

  //PROPERTIES
public:
  Q_PROPERTY(MissionPath * pathRw READ pathRw NOTIFY pathRwChanged)
  MissionPath * pathRw();
  Q_PROPERTY(MissionPath * pathApp READ pathApp NOTIFY pathAppChanged)
  MissionPath * pathApp();
  Q_PROPERTY(MissionPath * pathTA READ pathTA NOTIFY pathTAChanged)
  MissionPath * pathTA();
  Q_PROPERTY(QPointF rwEndPoint READ rwEndPoint WRITE setRwEndPoint NOTIFY rwEndPointChanged)
  QPointF rwEndPoint();
  void setRwEndPoint(QPointF ll);
  Q_PROPERTY(QPointF rwAppPoint READ rwAppPoint WRITE setRwAppPoint NOTIFY rwAppPointChanged)
  QPointF rwAppPoint();
  void setRwAppPoint(QPointF ll);
  Q_PROPERTY(double heading READ heading NOTIFY headingChanged)
  double heading();
private:
  MissionPath m_pathRw;
  MissionPath m_pathApp;
  MissionPath m_pathTA;
  QPointF m_rwEndPoint;
  QPointF m_rwAppPoint;
  double m_heading;
signals:
  void pathRwChanged();
  void pathAppChanged();
  void pathTAChanged();
  void rwEndPointChanged();
  void rwAppPointChanged();
  void headingChanged();
};
//=============================================================================
#endif
