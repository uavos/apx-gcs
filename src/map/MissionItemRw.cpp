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
#include "MissionItemRw.h"
#include "MissionItemField.h"
#include "QMandala.h"
#include "ItemRw.h"
#include "MapView.h"
//=============================================================================
MissionItemRw::MissionItemRw(MissionItemCategory<MissionItemRw> *parent)
 : MissionItemObject(parent,parent->model,parent->childName)
{
  f_turn=new MissionItemField(this,"turn",MissionItemField::dt_option,QStringList()<<"left"<<"right",tr("Landing pattern turn"));
  f_approach=new MissionItemField(this,"approach",MissionItemField::dt_distance,QStringList(),tr("Approach (straight path) length"));
  f_HMSL=new MissionItemField(this,"HMSL",MissionItemField::dt_distance,QStringList()<<"home_hmsl",tr("Runway ground altitude"));
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat,QStringList(),tr("Runway global position (latitude)"));
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon,QStringList(),tr("Runway global position (longitude)"));
  f_dN=new MissionItemField(this,"dN",MissionItemField::dt_distance,QStringList(),tr("Runway direction point (north)"));
  f_dE=new MissionItemField(this,"dE",MissionItemField::dt_distance,QStringList(),tr("Runway direction point (east)"));

  //default values
  QPointF ll(model->mapView->clkLL);
  if(ll.isNull()) ll=QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());
  f_dN->setValue(100);
  f_dE->setValue(300);
  f_approach->setValue(400);

  mapItem=new ItemRw(this);
}
//=============================================================================
QVariant MissionItemRw::data(int column,int role) const
{
  if(column!=tc_field || role!=Qt::DisplayRole) return MissionItem::data(column,role);
  return tr("Runway")+" #"+QString::number(row()+1);
}
//=============================================================================
QVariant MissionItemRw::value(void) const
{
  QStringList st;
  st.append(descr());
  st.append(QString("%1%2").arg(f_turn->data(tc_value).toString().at(0).toUpper()).arg((int)QMandala::angle360(QMandala::instance()->current->heading(f_dN->value().toDouble(),f_dE->value().toDouble()))));
  st.append(getDistanceString(f_approach->value().toDouble()));
  st.removeAll("");
  return st.join(' ');
}
//=============================================================================
QByteArray MissionItemRw::pack() const
{
  Mission::_item_rw v;
  v.hdr.type=Mission::mi_rw;
  v.hdr.option=f_turn->value().toUInt();
  v.lat=f_latitude->value().toFloat();
  v.lon=f_longitude->value().toFloat();
  v.hmsl=f_HMSL->value().toInt();
  v.dN=f_dN->value().toInt();
  v.dE=f_dE->value().toInt();
  v.approach=f_approach->value().toInt();
  return QByteArray((const char*)&v,sizeof(v));
}
int MissionItemRw::unpack(const QByteArray &ba)
{
  if(ba.size()<(int)sizeof(Mission::_item_rw))return 0;
  Mission::_item_rw *v=(Mission::_item_rw*)ba.data();
  if(v->hdr.type!=Mission::mi_rw)return 0;
  f_turn->setValue((uint)v->hdr.option);
  f_latitude->setValue(v->lat);
  f_longitude->setValue(v->lon);
  f_HMSL->setValue(v->hmsl);
  f_dN->setValue(v->dN);
  f_dE->setValue(v->dE);
  f_approach->setValue(v->approach);
  return sizeof(Mission::_item_rw);
}
//=============================================================================
