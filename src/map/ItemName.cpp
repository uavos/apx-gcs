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
#include "ItemName.h"
#include "QMandala.h"
#include "MapView.h"
//=============================================================================
ItemName::ItemName(MapView *view)
    :ItemBase(view,QMandala::instance()->current)
{
  setZValue(1000);
  setFlag(ItemIgnoresTransformations);


  QFont font;
  font.setPixelSize(24);
  font.setBold(true);

  txtName.setFont(font);
  txtName.setParentItem(this);
  txtName.setAlignment(Qt::AlignHCenter|Qt::AlignTop);
  txtName.setColor(Qt::yellow);
  txtName.setVisible(false);

  connect(QMandala::instance(),SIGNAL(uavNameChanged(QString)),this,SLOT(update_name(QString)));

  connect(view,SIGNAL(panned()),this,SLOT(viewPanned()));
  viewPanned();

  txtName.setCursor(Qt::PointingHandCursor);
  txtName.setFlag(QGraphicsItem::ItemIsFocusable);

  QObject::connect(&txtName,SIGNAL(clicked()),QMandala::instance(),SLOT(changeCurrentNext()));
}
//=============================================================================
void ItemName::update_name(QString s)
{
  txtName.setText(s);
  txtName.setVisible(QMandala::instance()->size()>0);
}
//=============================================================================
void ItemName::viewPanned()
{
  setPos(view->mapToScene(view->width()/2,5));
}
//=============================================================================

