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
#ifndef ItemRw_H
#define ItemRw_H
#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "ItemBase.h"
#include "ItemText.h"
class MissionItemRw;
//=============================================================================
class ItemRw : public ItemBase
{
  Q_OBJECT
public:
  ItemRw(MissionItemRw *modelItem);
  enum {Type=TypeItemRw};
  int type() const{return Type;}
  MissionItemRw *modelItem;
private:
  ItemBase *nedItem;
  QGraphicsLineItem *pathHdg,*pathHdgBg;
  ItemText txtNum,txtType;

  ItemBase *iTA_app;
  ItemText txtTA_app;

  bool blockModelUpdate;

  //venergy approach path
  QGraphicsLineItem *pathApp;
  QTimer timerVELD;
  QGraphicsPathItem *pathVELD;

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant &value);
private slots:
  void is_moved(void);
  void nedItemMoved(void);
  void updateWp(void);
  void updateHdg(void);
  void updateVELD(void);
  void updateFromModel();
  void viewScaled();
};
//=============================================================================
#endif
