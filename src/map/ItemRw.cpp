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
#include "ItemRw.h"
#include "QMandala.h"
#include "MapView.h"
#include "MissionModel.h"
//=============================================================================
ItemRw::ItemRw(MissionItemRw *modelItem)
    :ItemBase(modelItem->model->mapView,QMandala::instance()->current,QPixmap(":/icons/old/runway.png"),true),modelItem(modelItem)
{
  blockModelUpdate=false;

  nedItem=new ItemBase(view,mvar,QPixmap(":/icons/old/waypoint2.png"),true);
  addToScene(nedItem);
  connect(nedItem,SIGNAL(moved()),SLOT(nedItemMoved()));

  pathHdg=new QGraphicsLineItem();
  addToScene(pathHdg);
  QPen pen=QPen(Qt::white,4,Qt::DashLine);
  pen.setCosmetic(true);
  pathHdg->setPen(pen);
  pathHdg->setZValue(-0.9);
  pathHdgBg=new QGraphicsLineItem();
  addToScene(pathHdgBg);
  pen=QPen(QColor(10,10,10),5,Qt::SolidLine);
  pen.setCosmetic(true);
  pathHdgBg->setPen(pen);
  pathHdgBg->setZValue(-1);
  pathHdg->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
  pathHdgBg->setCacheMode(QGraphicsItem::DeviceCoordinateCache);


  txtType.setParentItem(this);
  txtType.setColor(QColor(55,121,197));
  txtType.setFontColor(Qt::white);

  txtNum.setParentItem(this);
  txtNum.setColor(QColor(55,121,197));
  txtNum.setFontColor(Qt::white);
  //txtNum.setColor(Qt::cyan);
  txtNum.setAlignment(Qt::AlignCenter);

  connect(this,SIGNAL(moved()),this,SLOT(is_moved()));
  connect(view,SIGNAL(scaled()),this,SLOT(updateHdg()));
  connect(view,SIGNAL(scaled()),this,SLOT(viewScaled()));

  iTA_app=new ItemBase(view,mvar,QPixmap(":/icons/old/waypoint2.png"));
  addToScene(iTA_app);
  iTA_app->setFlag(QGraphicsItem::ItemIsMovable,false);
  iTA_app->setFlag(QGraphicsItem::ItemIsSelectable,false);

  txtTA_app.setParentItem(iTA_app);
  txtTA_app.setFlags(ItemStacksBehindParent);
  txtTA_app.setColor(QColor(55,121,197));
  txtTA_app.setFontColor(Qt::white);
  //txtTA_app.setText("TA_app");

  //approach line
  pathApp=new QGraphicsLineItem();
  addToScene(pathApp);
  pathApp->setZValue(-100);
  pathApp->setCacheMode(QGraphicsItem::NoCache);
  QPen tpVE(Qt::white,0);
  tpVE.setStyle(Qt::DashLine);
  tpVE.setCosmetic(true);
  pathApp->setPen(tpVE);

  //venergy landing
  connect(&timerVELD,SIGNAL(timeout()),this,SLOT(updateVELD()));
  pathVELD=new QGraphicsPathItem();
  pathVELD->setCacheMode(QGraphicsItem::NoCache);
  addToScene(pathVELD);


  connect(modelItem->f_turn,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_approach,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_latitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_longitude,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_dN,SIGNAL(changed()),this,SLOT(updateFromModel()));
  connect(modelItem->f_dE,SIGNAL(changed()),this,SLOT(updateFromModel()));

  connect(modelItem->model,SIGNAL(addedRemoved()),this,SLOT(updateFromModel()));

  bind(SLOT(updateVELD()),QStringList()<<"mode"<<"turnR"<<"delta"<<"cmd_N"<<"cmd_E"<<"rwidx");

  update_all_binds();
  updateFromModel();
}
//=============================================================================
void ItemRw::viewScaled()
{
  setPosXY(view->mapToSceneLL(posLL.x(),posLL.y()));
}
//=============================================================================
void ItemRw::updateFromModel()
{
  txtType.setText(modelItem->value().toString().split(' ').first());
  txtTA_app.setText(modelItem->value().toString().split(' ').at(1));
  if(blockModelUpdate)return;
  QPointF ll=QPointF(modelItem->f_latitude->value().toDouble(),modelItem->f_longitude->value().toDouble());
  setPosLL(ll);
  updateWp();
  txtNum.setText(QString::number(modelItem->row()+1));
  txtTA_app.setVisible(modelItem->f_approach->value().toDouble()>100);
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
  updateVELD();
}
//=============================================================================
void ItemRw::is_moved(void)
{
  blockModelUpdate=true;
  modelItem->f_latitude->setValue(posLL.x());
  modelItem->f_longitude->setValue(posLL.y());
  blockModelUpdate=false;
}
//=============================================================================
void ItemRw::nedItemMoved(void)
{
  blockModelUpdate=true;
  QPointF ll=nedItem->getPosLL();
  QPointF hll=getPosLL();
  Vect hom(hll.x(),hll.y(),QMandala::instance()->current->home_pos[2]);
  Vect llh(ll.x(),ll.y(),QMandala::instance()->current->home_pos[2]);
  Point p=QMandala::instance()->current->llh2ne(llh,hom);

  modelItem->f_dN->setValue(p[0]);
  modelItem->f_dE->setValue(p[1]);

  updateWp();
  emit moved();
  blockModelUpdate=false;
}
//=============================================================================
void ItemRw::updateHdg(void)
{
  Point p=QMandala::instance()->current->ne2ll(Point(modelItem->f_dN->value().toDouble(),modelItem->f_dE->value().toDouble()),Vect(posLL.x(),posLL.y(),QMandala::instance()->current->home_pos[2]));
  nedItem->setPosLL(QPointF(p[0],p[1]));

  QLineF line(pos(),nedItem->pos());
  pathHdgBg->setLine(line);
  pathHdg->setLine(line);
  resetItemCache(pathHdg);
  resetItemCache(pathHdgBg);
}
//=============================================================================
void ItemRw::updateWp(void)
{
  updateHdg();
  double x,y;
  const double approach=modelItem->f_approach->value().toDouble();
  const double rwHDG=-QMandala::instance()->current->heading(modelItem->f_dN->value().toDouble(),modelItem->f_dE->value().toDouble());
  Point rwNE=QMandala::instance()->current->llh2ne(posLL.x(),posLL.y());
  Point appNE=rwNE+QMandala::instance()->current->rotate(-approach,0,rwHDG);
  Point ta_app(appNE-rwNE);
  x=view->mapMetersToScene(ta_app[1],posLL.x());
  y=view->mapMetersToScene(-ta_app[0],posLL.x());
  iTA_app->setPosXY(pos()+QPointF(x,y));
  pathApp->setLine(QLineF(pos(),iTA_app->pos()));
  updateVELD();
}
//=============================================================================
QVariant ItemRw::itemChange(GraphicsItemChange change, const QVariant &value)
{
  switch (change ){
  case ItemPositionHasChanged:
  case ItemPositionChange:
    updateWp();
    break;
    case ItemSelectedHasChanged:
      //txtType.setColor(value.toBool()?Qt::cyan:QColor());
      //txtTA_app.setColor(value.toBool()?Qt::cyan:QColor());
      break;
  default: break;
  }
  return ItemBase::itemChange(change, value);
}
//=============================================================================
void ItemRw::updateVELD(void)
{
  timerVELD.stop();

  QPainterPath p;
  double cR=QMandala::instance()->current->turnR;
  bool bLD=QMandala::instance()->current->mode==mode_LANDING && QMandala::instance()->current->rwidx==modelItem->row();
  if(!bLD){
    cR=modelItem->f_approach->value().toDouble()/2;
    if(cR<100)cR=100;
    if(modelItem->f_turn->data(MissionItem::tc_value).toString().toLower().startsWith("left"))cR=-cR;
  }
  const double sf=view->mapMetersToScene(1,getPosLL().x());
  const double r_s=-cR*sf;
  double r2=r_s*2.0;
  double rwHDG=QMandala::instance()->current->heading(modelItem->f_dN->value().toDouble(),modelItem->f_dE->value().toDouble());
  Point cRW=QMandala::instance()->current->llh2ne(Vect(posLL.x(),posLL.y(),QMandala::instance()->current->home_pos[2]));
  Point cP=QMandala::instance()->current->rotate(QMandala::instance()->current->cmd_NE-cRW,rwHDG);
  if((!bLD)||QMandala::instance()->current->distance(QMandala::instance()->current->cmd_NE-cRW)<1) p.addEllipse(0,-modelItem->f_approach->value().toDouble()*sf-r_s,r2,r2);
  else p.addEllipse(0,cP[0]*sf-r_s,r2,r2);
  if(bLD)p.addRect(-100.0*sf,0,sf*200.0,QMandala::instance()->current->delta*sf);
  pathVELD->setRotation(rwHDG+180.0);
  pathVELD->setPos(pos());
  pathVELD->setPath(p);

  QPen tpVE(Qt::white,0);
  tpVE.setStyle(Qt::DashLine);
  if(!bLD)tpVE.setColor(QColor(255,255,255,120));
  tpVE.setCosmetic(true);
  pathVELD->setPen(tpVE);

}
//=============================================================================
