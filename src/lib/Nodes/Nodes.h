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
#ifndef Nodes_H
#define Nodes_H
//=============================================================================
#include <QtCore>
#include "FactSystem.h"
#include "NodeItem.h"
class Vehicle;
//=============================================================================
class Nodes: public Fact
{
  Q_OBJECT

public:
  explicit Nodes(Vehicle *parent);

  Fact *f_request;

  Fact *f_list;

  Vehicle *vehicle;

  NodeItem * node(const QByteArray &sn){return snMap.value(sn);}
private:
  bool isBroadcast(const QByteArray &sn) const;
  NodeItem * nodeCheck(const QByteArray &sn);

  //sn lookup
  QHash<QByteArray,NodeItem*> snMap;
private slots:
  void search();

  //data comm
public slots:
  bool unpackService(const QByteArray &packet);


  //---------------------------------------
  // PROPERTIES
public:

protected:

signals:

};
//=============================================================================
#endif

