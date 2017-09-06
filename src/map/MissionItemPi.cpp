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
#include "MissionItemPi.h"
#include "MissionItemField.h"
#include "QMandala.h"
#include "ItemPi.h"
#include "MapView.h"
//=============================================================================
MissionItemPi::MissionItemPi(MissionItemCategory<MissionItemPi> *parent)
 : MissionItemObject(parent,parent->model,parent->childName)
{
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat,QStringList(),tr("Point of interest global position (latitude)"));
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon,QStringList(),tr("Point of interest global position (longitude)"));
  f_HMSL=new MissionItemField(this,"HMSL",MissionItemField::dt_distance,QStringList()<<"home_hmsl",tr("Camera view altitude"));
  f_turnR=new MissionItemField(this,"turnR",MissionItemField::dt_distance,QStringList(),tr("Loiter radius"));
  f_loops=new MissionItemField(this,"loops",MissionItemField::dt_byte,QStringList()<<"default",tr("Loiter loops limit"));
  f_time=new MissionItemField(this,"time",MissionItemField::dt_time,QStringList()<<"off",tr("Loiter time limit"));

  //default values
  QPointF ll(model->mapView->clkLL);
  if(ll.isNull()) ll=QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());
  if(row()>0) f_turnR->setValue(static_cast<MissionItemPi*>(parentItem->child(row()-1))->f_turnR->value());
  else{
    double turnR=QMandala::instance()->current->apcfg.value("turnR").toDouble();
    if(turnR==0)turnR=200;
    f_turnR->setValue(turnR);
  }

  mapItem=new ItemPi(this);
}
//=============================================================================
QVariant MissionItemPi::data(int column,int role) const
{
  if(column!=tc_field || role!=Qt::DisplayRole) return MissionItem::data(column,role);
  QStringList st;
  st.append("P"+QString::number(row()+1));
  return st.join(' ');
}
//=============================================================================
QVariant MissionItemPi::value(void) const
{
  QStringList st;
  double distanceT=QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(f_latitude->value().toDouble(),f_longitude->value().toDouble(),QMandala::instance()->current->home_pos[2])));
  st.append(getDistanceString(distanceT));
  st.append(descr());
  st.removeAll("");
  return st.join(' ');
}
//=============================================================================
QStringList MissionItemPi::getToolTip(void) const
{
  QStringList st=MissionItem::getToolTip();
  QString s=st.first();
  st.clear();
  st.append(s);
  st.append(QString("%1: %2 m").arg("DH").arg((uint)QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(f_latitude->value().toDouble(),f_longitude->value().toDouble(),QMandala::instance()->current->home_pos[2])))));
  return st;
}
//=============================================================================
QByteArray MissionItemPi::pack() const
{
  Mission::_item_pi v;
  v.hdr.type=Mission::mi_pi;
  v.lat=f_latitude->value().toFloat();
  v.lon=f_longitude->value().toFloat();
  v.hmsl=f_HMSL->value().toInt();
  v.turnR=f_turnR->value().toInt();
  v.loops=f_loops->value().toUInt();
  v.timeS=f_time->value().toUInt();
  return QByteArray((const char*)&v,sizeof(v));
}
int MissionItemPi::unpack(const QByteArray &ba)
{
  if(ba.size()<(int)sizeof(Mission::_item_pi))return 0;
  Mission::_item_pi *v=(Mission::_item_pi*)ba.data();
  if(v->hdr.type!=Mission::mi_pi)return 0;
  f_latitude->setValue(v->lat);
  f_longitude->setValue(v->lon);
  f_HMSL->setValue(v->hmsl);
  f_turnR->setValue(v->turnR);
  f_loops->setValue(v->loops);
  f_time->setValue(v->timeS);
  return sizeof(Mission::_item_pi);
}
//=============================================================================

