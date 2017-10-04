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
#ifndef ItemHome_H
#define ItemHome_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "ItemBase.h"
//=============================================================================
class ItemHome : public ItemBase
{
  Q_OBJECT
public:
  ItemHome(MapView *view);
  enum {Type=TypeItemHome};
  int type() const{return Type;}

  void setPosLL(QPointF ll);
private:
  QGraphicsPathItem *dHome;
  QGraphicsPathItem *dHomeERS;
private slots:
  //var bindings
  void update_pos();
  void update_apcfg();
};
//=============================================================================
#endif
