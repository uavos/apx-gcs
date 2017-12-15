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
#ifndef FactSystemApp_H
#define FactSystemApp_H
//=============================================================================
#include <QtCore>
#include <QtSql>
#include "Fact.h"
//=============================================================================
class FactSystemApp: public Fact
{
  Q_OBJECT

public:
  explicit FactSystemApp(QObject *parent=0);
  ~FactSystemApp();

  static FactSystemApp * instance() { return _instance; }

  //constants
  static const QString ApplicationSection;
  static const QString ToolsSection;

  static bool devMode()     { return _instance->m_dev; }
  static QString version()  { return _instance->m_version; }
  static QString branch()   { return _instance->m_branch; }
  static QSqlDatabase *db() { return _instance->m_db; }

protected:
  static FactSystemApp * _instance;

  bool m_dev;
  QString m_version;
  QString m_branch;
  QSqlDatabase * m_db;

public:
  // static app utils
  //----------------------------------
  Q_INVOKABLE static QString latToString(double v);
  Q_INVOKABLE static QString lonToString(double v);
  Q_INVOKABLE double static latFromString(QString s);
  Q_INVOKABLE double static lonFromString(QString s);
  Q_INVOKABLE static QString distanceToString(uint v);
  Q_INVOKABLE static QString timeToString(uint v);
  Q_INVOKABLE uint static timeFromString(QString s);

  Q_INVOKABLE static void toolTip(QString tooltip);
  Q_INVOKABLE static double limit(double v,double min,double max);
  Q_INVOKABLE static double angle360(double v);
  Q_INVOKABLE static double angle90(double v);
  Q_INVOKABLE static double angle(double v);
};
//=============================================================================
#endif
