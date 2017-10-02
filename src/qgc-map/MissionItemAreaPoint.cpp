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
#include "MissionModel.h"
#include "MissionItemAreaPoint.h"
#include "MissionItemField.h"
#include "QMandala.h"
//=============================================================================
MissionItemAreaPoint::MissionItemAreaPoint(MissionItemArea *parent, QString name)
 : MissionItemObject(parent,parent->model,name)
{
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat);
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon);

  //default values
  QPointF ll(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());

  connect(model,SIGNAL(addedRemoved()),this,SIGNAL(pathChanged()),Qt::QueuedConnection);

  color=parent->mi_type==Mission::mi_restricted?Qt::red:Qt::green;

  connect(f_latitude,SIGNAL(changed()),this,SIGNAL(pathChanged()));
  connect(f_longitude,SIGNAL(changed()),this,SIGNAL(pathChanged()));

  emit pathChanged();
}
//=============================================================================
QVariant MissionItemAreaPoint::data(int column,int role) const
{
  if(column!=tc_field || role!=Qt::DisplayRole) return MissionItem::data(column,role);
  QStringList st;
  st.append("vertex "+QString::number(row()+1));
  return st.join(' ');
}
//=============================================================================
QVariant MissionItemAreaPoint::value(void) const
{
  QStringList st;
  st.append(QMandala::distanceToString(DME()));
  st.append(descr());
  st.removeAll("");
  return st.join(' ');
}
//=============================================================================
QStringList MissionItemAreaPoint::getToolTip(void) const
{
  QStringList st=MissionItem::getToolTip();
  QString s=st.first();
  st.clear();
  st.append(s);
  st.append(QString("%1: %2 m").arg("DH").arg((uint)DME()));
  return st;
}
//=============================================================================
double MissionItemAreaPoint::DME() const
{
  return QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(f_latitude->value().toDouble(),f_longitude->value().toDouble(),QMandala::instance()->current->home_pos[2])));
}
//=============================================================================
MissionItemAreaPoint *MissionItemAreaPoint::prevItem() const
{
  if(row()==0) return NULL;
  return static_cast<MissionItemAreaPoint *>(parentItem->child(row()-1));
}
MissionItemAreaPoint *MissionItemAreaPoint::nextItem() const
{
  if(row()>=(parentItem->childCount()-1)) return NULL;
  return static_cast<MissionItemAreaPoint *>(parentItem->child(row()+1));
}
//=============================================================================
void MissionItemAreaPoint::remove(void)
{
  MissionItemObject::remove();
}
//=============================================================================

