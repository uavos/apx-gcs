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
#include "ItemTw.h"
#include "QMandala.h"
#include "MapView.h"
#include "MissionModel.h"
#include "MissionItemTw.h"
//=============================================================================
ItemTw::ItemTw(MissionItemTw *modelItem)
    :ItemBase(modelItem->model->mapView,QMandala::instance()->current,QPixmap(16,16),true),modelItem(modelItem)
{
  QPixmap ipm(pixmap().size());
  ipm.fill(Qt::transparent);
  setPixmap(ipm);
  setFlag(ItemHasNoContents);

  setZValue(99);
  last_num=-1;
  last_r=last_mode=last_wpidx=0;
  blockUpdateFromModel=false;

  path=new QGraphicsPathItem();
  addToScene(path);
  path->setZValue(97);
  path->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  txtNum.setParentItem(this);
  txtNum.setAlignment(Qt::AlignCenter);
  txtNum.setColor(Qt::darkYellow);
  txtNum.setFontColor(Qt::white);

  connect(this,SIGNAL(moved()),this,SLOT(is_moved()));
  connect(view,SIGNAL(scaled()),this,SLOT(updatePath()));
  connect(view,SIGNAL(scaled()),this,SLOT(viewScaled()));
  //mandalaUpdated(idx_wpcnt);

  connect(modelItem->f_latitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_longitude,SIGNAL(changed()),this,SLOT(updateFromModel()));

  connect(modelItem,SIGNAL(pathChanged()),this,SLOT(updatePath()));
  connect(this,SIGNAL(apcfgChanged()),this,SLOT(updateFromModel()));

  connect(modelItem->model,SIGNAL(addedRemoved()),this,SLOT(updateFromModel()));

  bind(SLOT(updatePath()),QStringList()<<"mode"<<"twidx");

  updateFromModel();
  update_all_binds();
}
//=============================================================================
void ItemTw::viewScaled()
{
  setPosXY(view->mapToSceneLL(posLL.x(),posLL.y()));
}
//=============================================================================
void ItemTw::updateFromModel()
{
  if(blockUpdateFromModel)return;
  QPointF ll=QPointF(modelItem->f_latitude->value().toDouble(),modelItem->f_longitude->value().toDouble());
  setPosLL(ll);
  txtNum.setText(QString::number(modelItem->row()+1));
}
//=============================================================================
void ItemTw::is_moved(void)
{
  blockUpdateFromModel=true;
  modelItem->f_latitude->setValue(posLL.x());
  modelItem->f_longitude->setValue(posLL.y());
  blockUpdateFromModel=false;
}
//=============================================================================
void ItemTw::updatePath()
{
  if(!modelItem->path.size())return;
  QPainterPath p;
  p.moveTo(0,0);
  //const double sf=view->transform().m11();
  double step_dist=view->mapMetersFromScene(view->mapToScene(10,0).x()-view->mapToScene(0,0).x(),posLL.x());
  QPointF pt0=modelItem->path.first();
  QPointF pt_scene=view->mapToSceneLL(pt0.x(),pt0.y());
  QPointF pathPos(pt_scene);
  foreach(const QPointF &pt,modelItem->path){
    double distance=QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(pt.x(),pt.y(),0),Vect(pt0.x(),pt0.y(),0)));
    if(distance<step_dist)continue;
    pt_scene=view->mapToSceneLL(pt.x(),pt.y());
    p.lineTo(pt_scene-pathPos);
    pt0=pt;
  }
  //if(pt_scene!=pos()) p.lineTo(pos()-pathPos);
  QPen pen;
  if(modelItem->row()==0) pen=QPen(QColor(0,20,0,100),1);
  else if(QMandala::instance()->current->mode!=mode_TAXI) pen=QPen(QColor(55,55,0,150),1,Qt::DashLine);
  else if((modelItem->row()-1)==QMandala::instance()->current->twidx) pen=QPen(QColor(0,200,0,200),5,Qt::DashLine);
  else pen=QPen(QColor(200,200,0,200),5,Qt::DashLine);
  pen.setCosmetic(true);
  path->setPen(pen);
  path->setPos(pathPos);
  path->setPath(p);
  resetItemCache(path);
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
