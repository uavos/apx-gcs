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
#ifndef Fact_H
#define Fact_H
//=============================================================================
#include "FactData.h"
//=============================================================================
class Fact: public FactData
{
  Q_OBJECT
public:
  explicit Fact(FactTree *parent, QString name, QString title, QString descr, ItemType treeItemType, DataType dataType);

  Q_INVOKABLE Fact * fact(const QString &factName) const;
  //Q_INVOKABLE Fact * byPath(const QString &itemNamePath) const;

public slots:
  virtual void trigger(void); //execute fact event (onClick)
signals:
  void triggered();
};
//=============================================================================
#endif
