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
#include "ItemWpt.h"
#include "QMandala.h"
#include "MapView.h"
#include "MissionModel.h"
#include "MissionItemWp.h"
//=============================================================================
ItemWpt::ItemWpt(MissionItemWp *modelItem)
 : ItemBase(modelItem->model->mapView,QMandala::instance()->current,QPixmap(":/icons/old/waypoint.png"),true),
   modelItem(modelItem)
{
  setZValue(99);
  last_num=-1;
  last_r=last_mode=last_wpidx=0;
  blockUpdateFromModel=false;

  path=new QGraphicsPathItem();
  addToScene(path);
  path->setZValue(97);
  //path->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  snap=new QGraphicsEllipseItem();
  addToScene(snap);
  snap->setPen(QPen(QColor(100,100,100),0));
  snap->setBrush(QBrush(QColor(10,100,100,20)));
  snap->setVisible(false);
  snap->setZValue(90);
  //snap->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  txtCmd.setParentItem(this);
  txtCmd.setColor(QColor(121,55,100));
  txtCmd.setFontColor(Qt::white);
  txtCmd.setAlignment(Qt::AlignHCenter|Qt::AlignTop);
  txtCmd.setVisible(true);

  txtInfo.setParentItem(this);
  //txtInfo.setAlignment(Qt::AlignLeft);
  txtInfo.setColor(QColor(55,121,100));//Qt::gray);
  txtInfo.setFontColor(Qt::white);
  txtInfo.setVisible(false);

  txtNum.setParentItem(this);
  txtNum.setAlignment(Qt::AlignCenter);
  txtNum.setColor(Qt::yellow);

  connect(this,SIGNAL(moved()),this,SLOT(is_moved()));
  connect(view,SIGNAL(scaled()),this,SLOT(updatePath()));
  connect(view,SIGNAL(scaled()),this,SLOT(viewScaled()));
  //mandalaUpdated(idx_wpcnt);

  connect(modelItem->f_type,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_latitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_longitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_altitude,SIGNAL(changed()),this,SLOT(updateFromModel()));

  connect(modelItem->f_actions,SIGNAL(changed()),this,SLOT(updateFromModel()));

  connect(modelItem,SIGNAL(pathChanged()),this,SLOT(updatePath()));
  connect(this,SIGNAL(apcfgChanged()),this,SLOT(updateFromModel()));

  connect(modelItem->model,SIGNAL(addedRemoved()),this,SLOT(updateFromModel()));

  updateFromModel();
  //update_all_binds();
}
//=============================================================================
void ItemWpt::viewScaled()
{
  setPosXY(view->mapToSceneLL(posLL.x(),posLL.y()));
}
//=============================================================================
void ItemWpt::updateFromModel()
{
  if(blockUpdateFromModel)return;
  QPointF ll=QPointF(modelItem->f_latitude->value().toDouble(),modelItem->f_longitude->value().toDouble());
  //qDebug()<<ll;
  /*if(ll.isNull()){
    if(view->clkLL.isNull()) ll=QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
    else ll=view->clkLL;
    modelItem->f_latitude->setValue(ll.x());
    modelItem->f_longitude->setValue(ll.y());
  }*/
  setPosLL(ll);
  txtNum.setText(QString::number(modelItem->row()+1));
  txtInfo.setText(modelItem->data(0).toString().split(' ').at(2));
  txtCmd.setText(modelItem->f_actions->value().toString());
  txtCmd.setVisible(!modelItem->f_actions->isZero());
  //double alt=QMandala::instance()->current->apcfg.value("altitude").toDouble();
  //txtInfo.setVisible(alt==0||modelItem->f_altitude->value()!=alt);
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
void ItemWpt::is_moved(void)
{
  blockUpdateFromModel=true;
  modelItem->f_latitude->setValue(posLL.x());
  modelItem->f_longitude->setValue(posLL.y());
  //setPos(view->mapToSceneLL(posLL.x(),posLL.y()));
  blockUpdateFromModel=false;
}
//=============================================================================
QVariant ItemWpt::itemChange(GraphicsItemChange change, const QVariant &value)
{
  switch (change ){
  case ItemPositionChange:{
    snap->setPos(value.toPointF());
    chkErrorWp();
  }break;
  case ItemSelectedHasChanged:
    chkErrorWp();
    snap->setVisible(isTargetWp()||value.toBool());
    txtInfo.setVisible(value.toBool());
    //txtInfo.setColor(value.toBool()?QColor(55,121,197):Qt::gray);
    break;
  default: break;
  }
  return ItemBase::itemChange(change, value);
}
//=============================================================================
bool ItemWpt::isTargetWp(void)
{
  return (QMandala::instance()->current->mode==mode_WPT)&&(modelItem->row()==(int)QMandala::instance()->current->wpidx);
}
//=============================================================================
bool ItemWpt::chkErrorWp(void)
{
  //check wpt pos
  bool wpOk=true;
  int iw=modelItem->row();
  Point wNE(QMandala::instance()->current->lla2ne(Vect(modelItem->f_latitude->value().toDouble(),modelItem->f_longitude->value().toDouble(),modelItem->f_altitude->value().toDouble())));
  for(int i=(iw>0?iw-1:iw); wpOk && (i<=(iw+1)) && (i<modelItem->parent()->childCount());i++){
    if(i==iw)continue;
    MissionItemWp *mi=static_cast<MissionItemWp*>(modelItem->parent()->child(i));
    Point iwNE(QMandala::instance()->current->lla2ne(Vect(mi->f_latitude->value().toDouble(),mi->f_longitude->value().toDouble(),mi->f_altitude->value().toDouble())));
    double d=QMandala::instance()->current->distance(wNE-iwNE);
    wpOk=d>(QMandala::instance()->current->apcfg.value("wpt_snap").toDouble()*4);
  }
  if(wpOk){
    snap->setPen(QPen(QColor(100,100,100),0));
    snap->setBrush(QBrush(QColor(10,100,100,20)));
  }else{
    snap->setPen(QPen(QColor(255,50,50),0));
    snap->setBrush(QBrush(QColor(200,100,100,100)));
  }
  return !wpOk;
}
//=============================================================================
//=============================================================================
void ItemWpt::updatePath()
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
  if(modelItem->row()==0) pen=QPen(QColor(0,200,0,200),1.5);
  else if(!modelItem->wptReached) pen=QPen(QColor(255,0,0,200),5);
  else if(modelItem->wptWarning) pen=QPen(QColor(255,200,0,200),5);
  else if(modelItem->f_type->data(MissionItem::tc_value).toString().toLower()=="line") pen=QPen(QColor(150,150,255,200),5);
  else pen=QPen(QColor(0,255,255,150),3);
  pen.setCosmetic(true);
  path->setPen(pen);
  path->setPos(pathPos);
  path->setPath(p);
  resetItemCache(path);
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
}
//=============================================================================
