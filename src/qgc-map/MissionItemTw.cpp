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
#include "MissionItemTw.h"
#include "MissionItemField.h"
#include "QMandala.h"
//=============================================================================
MissionItemTw::MissionItemTw(MissionItemCategory<MissionItemTw> *parent)
 : MissionItemObject(parent,parent->model,parent->childName)
{
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat);
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon);

  //default values
  QPointF ll(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());

  connect(model,SIGNAL(addedRemoved()),this,SLOT(updatePath()),Qt::QueuedConnection);

  connect(f_latitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_longitude,SIGNAL(changed()),this,SLOT(updatePath()));

  connect(QMandala::instance()->current,SIGNAL(apcfgChanged()),this,SLOT(updatePath()),Qt::QueuedConnection);

  updatePath();

}
//=============================================================================
QVariant MissionItemTw::data(int column,int role) const
{
  if(column!=tc_field || role!=Qt::DisplayRole) return MissionItem::data(column,role);
  QStringList st;
  st.append("T"+QString::number(row()+1));
  return st.join(' ');
}
//=============================================================================
QVariant MissionItemTw::value(void) const
{
  QStringList st;
  st.append(QMandala::distanceToString(m_distance));
  st.append(descr());
  st.removeAll("");
  return st.join(' ');
}
//=============================================================================
QStringList MissionItemTw::getToolTip(void) const
{
  QStringList st=MissionItem::getToolTip();
  QString s=st.first();
  st.clear();
  st.append(s);
  st.append(QString("%1: %2 m").arg("DH").arg((uint)QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(f_latitude->value().toDouble(),f_longitude->value().toDouble(),QMandala::instance()->current->home_pos[2])))));
  st.append(QString("%1: %2 m").arg("DW").arg((uint)m_distance));
  return st;
}
//=============================================================================
MissionPath * MissionItemTw::path()
{
  return &m_path;
}
int MissionItemTw::distance() const
{
  return m_distance;
}
//=============================================================================
MissionItemTw *MissionItemTw::prevItem() const
{
  if(row()==0) return NULL;
  return static_cast<MissionItemTw *>(parentItem->child(row()-1));
}
MissionItemTw *MissionItemTw::nextItem() const
{
  if(row()>=(parentItem->childCount()-1)) return NULL;
  return static_cast<MissionItemTw *>(parentItem->child(row()+1));
}
//=============================================================================
void MissionItemTw::updatePath()
{
  double spd=QMandala::instance()->current->apcfg.value("spd_cruise").toDouble();
  if(spd<=0)spd=22;
  int idist=0;
  QList<QPointF> plist;
  MissionItemTw *prevWp=prevItem();
  QPointF dest(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  QPointF pt;
  if(!prevWp){
    pt=QPointF(QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]));
  }else pt=QPointF(prevWp->f_latitude->value().toDouble(),prevWp->f_longitude->value().toDouble());

  plist.append(pt);
  plist.append(dest);
  idist=Mandala::distance(Point(pt.x(),pt.y()),Point(dest.x(),dest.y()));

  if(m_distance!=idist){
    m_distance=idist;
    emit distanceChanged();
  }

  //calc additional information and updates
  bool doUpdNext=false;
  //update path
  m_path.setPath(plist);
  //check to propagate updates to next wpts
  MissionItemTw *nextWp=nextItem();
  if(nextWp){
    doUpdNext|=nextWp->path()->path().size()<2 || nextWp->path()->path().first()!=dest;
    if(doUpdNext)nextWp->updatePath();
  }
}
//=============================================================================
QByteArray MissionItemTw::pack() const
{
  Mission::_item_tw v;
  v.hdr.type=Mission::mi_tw;
  v.lat=f_latitude->value().toFloat();
  v.lon=f_longitude->value().toFloat();
  return QByteArray((const char*)&v,sizeof(v));
}
int MissionItemTw::unpack(const QByteArray &ba)
{
  if(ba.size()<(int)sizeof(Mission::_item_tw))return 0;
  Mission::_item_tw *v=(Mission::_item_tw*)ba.data();
  if(v->hdr.type!=Mission::mi_tw)return 0;
  f_latitude->setValue(v->lat);
  f_longitude->setValue(v->lon);
  return sizeof(Mission::_item_tw);
}
//=============================================================================

