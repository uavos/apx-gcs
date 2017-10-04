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
#include "ItemAreaPoint.h"
#include "QMandala.h"
#include "MapView.h"
#include "MissionModel.h"
#include "MissionItemAreaPoint.h"
//=============================================================================
ItemAreaPoint::ItemAreaPoint(MissionItemAreaPoint *modelItem, QColor color)
: ItemBase(modelItem->model->mapView,QMandala::instance()->current,QPixmap(16,16),true),
  modelItem(modelItem),color(color)
{
  QPixmap ipm(pixmap().size());
  ipm.fill(Qt::transparent);
  setPixmap(ipm);
  //setFlag(ItemHasNoContents);

  setZValue(99);
  last_num=-1;
  last_r=last_mode=last_wpidx=0;
  blockUpdateFromModel=false;

  txtNum.setParentItem(this);
  txtNum.setAlignment(Qt::AlignCenter);
  txtNum.setColor(Qt::darkGray);
  txtNum.setFontColor(Qt::yellow);

  vertexCircle.setParentItem(this);
  vertexCircle.setRect(boundingRect());
  vertexCircle.setVisible(false);

  itemSelected(false);

  connect(this,SIGNAL(selected(bool)),this,SLOT(itemSelected(bool)));

  connect(this,SIGNAL(moved()),this,SLOT(is_moved()));
  connect(view,SIGNAL(scaled()),this,SLOT(viewScaled()));

  connect(modelItem->f_latitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_longitude,SIGNAL(changed()),this,SLOT(updateFromModel()));

  connect(this,SIGNAL(apcfgChanged()),this,SLOT(updateFromModel()));

  connect(modelItem->model,SIGNAL(addedRemoved()),this,SLOT(updateFromModel()));

  updateFromModel();
  update_all_binds();
}
//=============================================================================
void ItemAreaPoint::itemSelected(bool v)
{
  QColor c=color;
  c.setAlpha(220);
  vertexCircle.setPen(v?c:c.lighter());
  c.setAlpha(100);
  vertexCircle.setBrush(v?c:c.lighter());
  vertexCircle.setVisible(v);
}
//=============================================================================
void ItemAreaPoint::viewScaled()
{
  setPosXY(view->mapToSceneLL(posLL.x(),posLL.y()));
}
//=============================================================================
void ItemAreaPoint::updateFromModel()
{
  if(blockUpdateFromModel)return;
  QPointF ll=QPointF(modelItem->f_latitude->value().toDouble(),modelItem->f_longitude->value().toDouble());
  setPosLL(ll);
  txtNum.setText(QString::number(modelItem->row()+1));
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
void ItemAreaPoint::is_moved(void)
{
  blockUpdateFromModel=true;
  modelItem->f_latitude->setValue(posLL.x());
  modelItem->f_longitude->setValue(posLL.y());
  blockUpdateFromModel=false;
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
