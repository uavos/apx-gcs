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
#ifndef MissionItemField_H
#define MissionItemField_H
#include "MissionItem.h"
//=============================================================================
class MissionItemField: public MissionItem
{
  Q_OBJECT
public:
  MissionItemField(MissionItem *parent,QString name, uint ftype, QStringList opts=QStringList(),QString descr=QString());
  //override MissionItem
  QVariant data(int column,int role = Qt::DisplayRole) const;
  QVariant value(void) const;
  bool setValue(QVariant value);
  Q_INVOKABLE bool isModified(void) const;
  Qt::ItemFlags flags(int column) const;

  //methods
  bool isZero(void) const;
  void restore(void);

  //data types
  enum{dt_default,dt_option,dt_varmsk,dt_distance,dt_angle,dt_lat,dt_lon,dt_script,dt_float,dt_byte,dt_time};

  //field parameters
  uint              ftype;
  QStringList       opts;

  QVariant m_value,m_bkp;
protected:
  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);
  QByteArray md5() const;
  void remove(void){} //no remove
public slots:
  void backup();
  void invalidate();
};
//=============================================================================
class MissionItemFieldGroup: public MissionItem
{
  Q_OBJECT
public:
  MissionItemFieldGroup(MissionItem *parent,QString name,QString caption=QString(),QString descr=QString());
protected:
  void saveToXml(QDomNode dom) const;
  void loadFromXml(QDomNode dom);
  void remove(void){} //no remove
};
//=============================================================================
#endif
