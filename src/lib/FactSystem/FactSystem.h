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
#include "FactSystemApp.h"
#include "FactSystemJS.h"
class QJSEngine;
//=============================================================================
class FactSystem: public FactSystemJS
{
  Q_OBJECT
public:
  //root
  explicit FactSystem(QObject *parent=0);

  static FactSystem * instance() { return static_cast<FactSystem*>(_instance); }

  //methods
  Q_INVOKABLE void sound(const QString &v) { emit playSoundEffect(v); }

  Q_INVOKABLE QJSValue jsexec(const QString &s) { return FactSystemJS::jsexec(s); }


signals:
  void playSoundEffect(const QString &v);
};
//=============================================================================
#endif
