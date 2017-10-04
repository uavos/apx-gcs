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
#ifndef MissionItemArea_H
#define MissionItemArea_H
#include "MissionItemObject.h"
class ItemBase;
class MissionModel;
class MissionItemField;
class MissionItemAreaPoint;
//=============================================================================
class MissionItemArea: public MissionItemObject
{
  Q_OBJECT
public:
  MissionItemArea(MissionItemCategory<MissionItemArea> *parent);
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;
  QStringList getToolTip(void) const;

  Mission::_item_type mi_type; //binary packet data hdr byte

  QString areaName;
  QString childName;

  QList<QPointF> path; //saved path (lat,lon)

  QList<MissionItemAreaPoint *> selectedPoints(void) const;
protected:
  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);
  QByteArray pack() const;
  int unpack(const QByteArray &ba);

  void mapItemSelected(bool v);
  MissionItemAreaPoint * addPoint();
  void addPointToSelectedObject();
private:
  double distance;
private slots:
  void updatePath();

signals:
  void pathChanged();
};
//=============================================================================
#endif
