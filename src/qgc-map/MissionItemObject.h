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
#ifndef MissionItemObject_H
#define MissionItemObject_H
//=============================================================================
#include "MissionItem.h"
#include "MissionItemCategory.h"
class MissionModel;
class MissionItemField;
//=============================================================================
class MissionItemObject: public MissionItem
{
  Q_OBJECT
public:
  MissionItemObject(MissionItem *parent, MissionModel *model, QString childName);
  MissionModel *model;

protected:
  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);

  Qt::ItemFlags flags(int column) const;
protected slots:
  virtual void mapItemSelected(bool v);
  void modelSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
public slots:
  virtual void addPointToSelectedObject(){}
};
//=============================================================================
#endif
