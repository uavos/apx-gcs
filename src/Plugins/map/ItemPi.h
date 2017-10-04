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
#ifndef ItemPi_H
#define ItemPi_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "ItemBase.h"
#include "ItemText.h"
class MissionItemPi;
//=============================================================================
class ItemPi : public ItemBase
{
  Q_OBJECT
public:
  ItemPi(MissionItemPi *modelItem);
  enum {Type=TypeItemPi};
  int type() const{return Type;}

  QGraphicsEllipseItem *circle;
  ItemText txtNum;
  ItemText txtInfo;

  bool blockUpdateFromModel;
  MissionItemPi *modelItem;

private slots:
  void is_moved(void);
  void updateFromModel();
  void updateCircle();
  void viewScaled();
};
//=============================================================================
#endif
