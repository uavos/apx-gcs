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
#include "ItemHome.h"
#include "QMandala.h"
#include "MapView.h"
//=============================================================================
ItemHome::ItemHome(MapView *view)
:ItemBase(view,QMandala::instance()->current,QPixmap(":/icons/old/home.png"))
{
  setToolTip(tr("Home position"));

  dHome=new QGraphicsPathItem();
  dHome->setPen(QPen(QColor(100,100,255),0,Qt::SolidLine,Qt::FlatCap,Qt::MiterJoin));
  dHome->setBrush(QBrush(QColor(100,100,255,100)));
  addToScene(dHome);

  dHomeERS=new QGraphicsPathItem();
  dHomeERS->setPen(QPen(QColor(255,80,80),0,Qt::SolidLine,Qt::FlatCap,Qt::MiterJoin));
  dHomeERS->setBrush(QBrush(QColor(255,100,100,100)));
  addToScene(dHomeERS);

  bind(SLOT(update_pos()),QStringList()<<"status_gps"<<"home_lat"<<"home_lon");
  bind(SLOT(update_apcfg()),QStringList()<<"home_lat"<<"home_lon");
  connect(this,SIGNAL(apcfgChanged()),this,SLOT(update_apcfg()));

  update_all_binds();
}
//=============================================================================
void ItemHome::setPosLL(QPointF ll)
{
  ItemBase::setPosLL(ll);
  dHome->setPos(pos());
  dHomeERS->setPos(pos());
}
//=============================================================================
void ItemHome::update_pos()
{
  QPointF homePos=QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  if((QMandala::instance()->current->status&status_gps)||(homePos!=QPointF(0,0)))
    setPosLL(homePos);
}
//=============================================================================
void ItemHome::update_apcfg()
{
  double v_dHome=QMandala::instance()->current->apcfg.value("sf_dHome").toDouble();
  if(v_dHome>0){
    double r=view->mapMetersToScene(v_dHome,posLL.x());
    QPainterPath path;
    path.addEllipse(-r,-r,r*2,r*2);
    r=r*1.1;
    path.addEllipse(-r,-r,r*2,r*2);
    dHome->setPath(path);
    dHome->setVisible(true);
  }else dHome->setVisible(false);
  double v_dHomeERS=QMandala::instance()->current->apcfg.value("sf_dHomeERS").toDouble();
  if(v_dHomeERS>0){
    double r=view->mapMetersToScene(v_dHomeERS,posLL.x());
    QPainterPath path;
    path.addEllipse(-r,-r,r*2,r*2);
    r=r*1.1;
    path.addEllipse(-r,-r,r*2,r*2);
    dHomeERS->setPath(path);
    dHomeERS->setVisible(true);
  }else dHomeERS->setVisible(false);
}
//=============================================================================

