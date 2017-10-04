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
#include "ItemWind.h"
#include "QMandala.h"
#include "MapView.h"
//=============================================================================
ItemWind::ItemWind(MapView *view)
    :ItemBase(view,QMandala::instance()->current)
{
  setZValue(1000);
  setFlag(ItemIgnoresTransformations);

  QFont font;
  font.setPixelSize(18);
  font.setBold(true);

  windHdg=new ItemBase(view,mvar,QPixmap(":/icons/old/wind_arrow.png"));
  windHdg->setParentItem(this);
  windHdg->setPos(-windHdg->boundingRect().height()/2,windHdg->boundingRect().height()/2+font.pixelSize()+2);
  windHdg->setTransformationMode(Qt::SmoothTransformation);

  txtWindSpd.setFont(font);
  txtWindSpd.setParentItem(this);
  txtWindSpd.setAlignment(Qt::AlignRight|Qt::AlignTop);
  //txtWindSpd.setPos(-windHdg->boundingRect().height(),font.pixelSize());
  txtWindSpd.setColor(Qt::yellow);

  connect(view,SIGNAL(panned()),this,SLOT(viewPanned()));

  bind(SLOT(update_wind()),QStringList()<<"windSpd"<<"windHdg");

  update_all_binds();
  viewPanned();
}
//=============================================================================
void ItemWind::update_wind()
{
  txtWindSpd.setText(QString().sprintf("%.1fm/s ",QMandala::instance()->current->windSpd));
  int a=QMandala::instance()->current->windHdg;
  if(windHdg->rotation()!=a)windHdg->setRotation(a);
}
//=============================================================================
void ItemWind::viewPanned()
{
  setPos(view->mapToScene(view->width()-5,5));
}
//=============================================================================

