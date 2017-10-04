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
class ItemBase;
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
};
//=============================================================================
#endif
