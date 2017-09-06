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
#include "MissionPath.h"
#include "Mandala.h"
//=============================================================================
MissionPath::MissionPath(QObject *parent)
  : QObject(parent)
{
}
//=============================================================================
void MissionPath::setPath(const QList<QPointF> &v)
{
  if(m_path==v)return;
  m_path=v;
  emit pathChanged();
}
//=============================================================================
const QList<QPointF> & MissionPath::path() const
{
  return m_path;
}
//=============================================================================
void MissionPath::setCircle(const QPointF &center,const double radius)
{
  QList<QPointF> plist;
  Point llC(center.x(),center.y());
  for(double a=0;a<360;a+=5){
    Point ll=Mandala::destination(llC,a,radius);
    plist.append(QPointF(ll[0],ll[1]));
  }
  plist.append(plist.first());
  setPath(plist);
}
//=============================================================================
