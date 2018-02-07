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
#ifndef MissionXml_H
#define MissionXml_H
//=============================================================================
#include <QtCore>
#include <QtXml>
class VehicleMission;
class MissionGroup;
class Fact;
//=============================================================================
class MissionXml: public QObject
{
  Q_OBJECT

public:
  explicit MissionXml(VehicleMission *parent);

  QDomDocument write() const;
  void write(QDomNode dom) const;
  void write(QDomNode dom, MissionGroup *group, const QString &sectionName, const QString &elementName) const;
  void write(QDomNode dom, Fact *fact) const;

  bool read(QDomNode dom) const;
  int read(QDomNode dom, MissionGroup *group, const QString &sectionName, const QString &elementName) const;
  int read(QDomNode dom, Fact *fact) const;

private:
  VehicleMission *mission;
  int format;
  static QMap<QString,QString> xmlMap;
};
//=============================================================================
#endif

