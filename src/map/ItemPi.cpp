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
#include "ItemPi.h"
#include "QMandala.h"
#include "MapView.h"
#include "MissionModel.h"
#include "MissionItemPi.h"
//=============================================================================
ItemPi::ItemPi(MissionItemPi *modelItem)
    :ItemBase(modelItem->model->mapView,QMandala::instance()->current,QPixmap(16,16),true),modelItem(modelItem)
{
  QPixmap ipm(pixmap().size());
  ipm.fill(Qt::transparent);
  setPixmap(ipm);
  setFlag(ItemHasNoContents);

  setZValue(99);
  blockUpdateFromModel=false;

  circle=new QGraphicsEllipseItem();
  circle->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
  //circle->setParentItem(this);
  addToScene(circle);

  txtNum.setParentItem(this);
  txtNum.setAlignment(Qt::AlignCenter);
  txtNum.setColor(QColor(55,121,197));
  txtNum.setFontColor(Qt::white);

  txtInfo.setParentItem(this);
  txtInfo.setAlignment(Qt::AlignTop|Qt::AlignHCenter);
  txtInfo.setColor(QColor(55,121,100));//Qt::gray);
  txtInfo.setFontColor(Qt::white);
  txtInfo.setVisible(false);
  //txtInfo.setPos(txtNum.x(),txtNum.boundingRect().height()+2);


  connect(this,SIGNAL(moved()),this,SLOT(is_moved()));
  connect(view,SIGNAL(scaled()),this,SLOT(updateCircle()));
  connect(view,SIGNAL(scaled()),this,SLOT(viewScaled()));
  //mandalaUpdated(idx_wpcnt);

  connect(modelItem->f_latitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_longitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_loops,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_time,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->model,SIGNAL(addedRemoved()),this,SLOT(updateFromModel()));

  connect(modelItem->f_turnR,SIGNAL(changed()),this,SLOT(updateCircle()),Qt::QueuedConnection);
  connect(this,SIGNAL(selected(bool)),this,SLOT(updateCircle()),Qt::QueuedConnection);

  bind(SLOT(updateCircle()),QStringList()<<"mode"<<"piidx");

  updateFromModel();
  update_all_binds();
}
//=============================================================================
void ItemPi::viewScaled()
{
  setPosXY(view->mapToSceneLL(posLL.x(),posLL.y()));
}
//=============================================================================
void ItemPi::updateFromModel()
{
  if(blockUpdateFromModel)return;
  QPointF ll=QPointF(modelItem->f_latitude->value().toDouble(),modelItem->f_longitude->value().toDouble());
  setPosLL(ll);
  circle->setPos(pos());
  QString s;
  if(modelItem->f_loops->value().toUInt()>0)
    s+="L"+modelItem->f_loops->value().toString();
  if(modelItem->f_time->value().toUInt()>0)
    s+="T"+modelItem->f_time->data(1,Qt::DisplayRole).toString();
  txtInfo.setText(s);
  txtNum.setText(modelItem->data(0,Qt::DisplayRole).toString());
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
void ItemPi::is_moved(void)
{
  circle->setPos(view->mapToSceneLL(posLL.x(),posLL.y()));
  blockUpdateFromModel=true;
  modelItem->f_latitude->setValue(posLL.x());
  modelItem->f_longitude->setValue(posLL.y());
  blockUpdateFromModel=false;
}
//=============================================================================
void ItemPi::updateCircle()
{
  double r=fabs(modelItem->f_turnR->value().toDouble());
  if(r==0)r=10;
  r=view->mapMetersToScene(r,getPosLL().x());
  circle->setRect(QRectF(-r,-r,r*2,r*2));
  QPen pen(QColor(100,100,255));
  pen.setCosmetic(true);
  QBrush brush(QColor(10,10,100,20));
  if(modelItem->row()==QMandala::instance()->current->piidx && QMandala::instance()->current->mode==mode_STBY){
    pen.setColor(Qt::green);
  }
  if(isSelected()){
    brush.setColor(QColor(0,255,0,20));
  }
  circle->setPen(pen);
  circle->setBrush(brush);
  txtInfo.setVisible(isSelected());
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
