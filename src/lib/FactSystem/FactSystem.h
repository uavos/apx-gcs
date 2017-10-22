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
#ifndef FactSystem_H
#define FactSystem_H
//=============================================================================
#include <QtCore>
#include <QtQuick>
#include "Fact.h"
//=============================================================================
class FactSystem: public QObject
{
  Q_OBJECT
public:
  //root
  explicit FactSystem(QObject *parent=0);
  ~FactSystem();

  //methods
  static Fact * tree();
  static void appendTree(Fact *tree);
  static void syncJS(QQmlEngine *e);

  static QVariant value(const QString &name);

  //constants
  static const QString ApplicationSection;
  static const QString MandalaSection;


  //static helpers
  static FactSystem * _instance;
  Fact * _tree;
  QJSValue _syncJS(QQmlEngine *e, Fact *factItem, QJSValue parent); //recursive
};
//=============================================================================
#endif
