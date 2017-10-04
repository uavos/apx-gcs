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
#include "ItemArea.h"
#include "QMandala.h"
#include "MapView.h"
#include "MissionModel.h"
#include "MissionItemArea.h"
//=============================================================================
ItemArea::ItemArea(MissionItemArea *modelItem, QColor color)
: ItemBase(modelItem->model->mapView,QMandala::instance()->current,QPixmap(16,16),false),
  modelItem(modelItem),color(color)
{
  QPixmap ipm(pixmap().size());
  ipm.fill(Qt::transparent);
  setPixmap(ipm);
  //setFlag(ItemHasNoContents);
  setFlag(ItemIsSelectable);
  setCursor(Qt::PointingHandCursor);

  setZValue(99);
  last_num=-1;
  last_r=last_mode=last_wpidx=0;

  path=new QGraphicsPathItem();
  addToScene(path);
  path->setZValue(97);
  //path->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

  txtNum.setParentItem(this);
  txtNum.setAlignment(Qt::AlignCenter);
  QFont font;
  font.setPixelSize(10);
  txtNum.setFont(font);
  txtNum.setColor(QColor(254,227,110));
  txtNum.setFontColor(Qt::black);


  itemSelected(false);

  connect(this,SIGNAL(selected(bool)),this,SLOT(itemSelected(bool)));


  //connect(view,SIGNAL(scaled()),this,SLOT(updatePath()));
  //connect(view,SIGNAL(scaled()),this,SLOT(viewScaled()));

  connect(modelItem,SIGNAL(pathChanged()),this,SLOT(updatePath()));

  updatePath();
}
//=============================================================================
void ItemArea::itemSelected(bool v)
{
  QColor c=color;
  c.setAlpha(220);
  QPen pen=QPen(v?c:c.lighter(),1);
  c.setAlpha(100);
  pen.setCosmetic(true);
  path->setPen(pen);
  path->setBrush(v?c:c.lighter());
}
//=============================================================================
QRectF ItemArea::boundingRect() const
{
  return txtNum.boundingRect().translated(txtNum.pos());
}
//=============================================================================
void ItemArea::viewScaled()
{
  //setPosXY(view->mapToSceneLL(posLL.x(),posLL.y()));
}
//=============================================================================
void ItemArea::updatePath()
{
  if(!modelItem->path.size())return;
  QPainterPath p;
  //const double sf=view->transform().m11();
  QPointF pt0=modelItem->path.first();
  QPointF pt_scene=view->mapToSceneLL(pt0.x(),pt0.y());
  QPointF pathPos(pt_scene);
  int i=0;
  foreach(const QPointF &pt,modelItem->path){
    pt_scene=view->mapToSceneLL(pt.x(),pt.y());
    if(i++)p.lineTo(pt_scene-pathPos);
    else p.moveTo(0,0);
  }
  if(i)p.lineTo(0,0);
  path->setPos(pathPos);
  path->setPath(p);
  resetItemCache(path);
  setToolTip(modelItem->data(0,Qt::ToolTipRole).toString());
  //text position update
  //setPosLL(modelItem->path.first());
  QRectF r=p.controlPointRect();
  setPos(pathPos+r.center());
  txtNum.setText(modelItem->areaName+" "+QString::number(modelItem->row()+1));
  //qDebug()<<"updatePath";
}
//=============================================================================
