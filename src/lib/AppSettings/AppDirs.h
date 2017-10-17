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
#ifndef AppDirs_H
#define AppDirs_H
#include <QtCore>
//=============================================================================
class AppDirs: public QObject
{
  Q_OBJECT
public:
  explicit AppDirs(QObject *parent=0) : QObject(parent) {}

  static QDir user();         // local user files (Documents)
  static QDir res();          // resources
  static QDir plugins();      // plugins
  static QDir userPlugins();  // user plugins
  static QDir telemetry();    // saved flight data
  static QDir maps();         // maps and tilesets
  static QDir lang();         // translations
  static QDir missions();     // saved flight plans
  static QDir configs();      // user saved nodes config files
  static QDir nodes();        // nodes backups
  static QDir scripts();      // user saved scripts
};
//=============================================================================
#endif

