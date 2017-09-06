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
//=============================================================================
MissionItemRw::MissionItemRw(MissionItemCategory<MissionItemRw> *parent)
 : MissionItemObject(parent,parent->model,parent->childName)
{
  f_turn=new MissionItemField(this,"turn",MissionItemField::dt_option,QStringList()<<"left"<<"right",tr("Landing pattern turn"));
  f_approach=new MissionItemField(this,"approach",MissionItemField::dt_distance,QStringList(),tr("Approach (straight path) length"));
  f_HMSL=new MissionItemField(this,"HMSL",MissionItemField::dt_distance,QStringList()<<"home_hmsl",tr("Runway ground altitude"));
  f_dN=new MissionItemField(this,"dN",MissionItemField::dt_distance,QStringList(),tr("Runway direction point (north)"));
  f_dE=new MissionItemField(this,"dE",MissionItemField::dt_distance,QStringList(),tr("Runway direction point (east)"));
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat,QStringList(),tr("Runway global position (latitude)"));
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon,QStringList(),tr("Runway global position (longitude)"));

  //default values
  QPointF ll(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());
  f_dN->setValue(100);
  f_dE->setValue(300);
  f_approach->setValue(400);

  connect(f_latitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_longitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_dN,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_dE,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_approach,SIGNAL(changed()),this,SLOT(updatePath()));

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
  st.append(QMandala::distanceToString(f_approach->value().toDouble()));
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
MissionPath * MissionItemRw::pathRw()
{
  return &m_pathRw;
}
MissionPath * MissionItemRw::pathApp()
{
  return &m_pathApp;
}
MissionPath * MissionItemRw::pathTA()
{
  return &m_pathTA;
}
QPointF MissionItemRw::rwEndPoint()
{
  return m_rwEndPoint;
}
void MissionItemRw::setRwEndPoint(QPointF ll)
{
  if(m_rwEndPoint==ll)return;
  Vect home_llh(f_latitude->value().toDouble(),f_longitude->value().toDouble(),f_HMSL->value().toDouble());
  Vect llh(ll.x(),ll.y(),f_HMSL->value().toDouble());
  Point ne=QMandala::instance()->current->llh2ne(llh,home_llh);
  if(QMandala::instance()->current->distance(ne)>5000){
    ne=QMandala::instance()->current->rotate(5000,0,-QMandala::instance()->current->heading(ne));
  }
  f_dN->setValue(ne[0]);
  f_dE->setValue(ne[1]);
  updateHeading();
}
QPointF MissionItemRw::rwAppPoint()
{
  return m_rwAppPoint;
}
void MissionItemRw::setRwAppPoint(QPointF ll)
{
  if(m_rwAppPoint==ll)return;
  Vect home_llh(f_latitude->value().toDouble(),f_longitude->value().toDouble(),f_HMSL->value().toDouble());
  Vect llh(ll.x(),ll.y(),f_HMSL->value().toDouble());
  Point ne=QMandala::instance()->current->llh2ne(llh,home_llh);
  ne=QMandala::instance()->current->rotate(ne,m_heading+180.0);
  if(fabs(ne[1])>(f_approach->value().toDouble()/2.0)){
    //switch turn direction
    f_turn->setValue(ne[1]>0?0:1);
  }
  int dist=ne[0];//abs(QMandala::instance()->current->boundAngle(QMandala::instance()->current->heading(ne)-m_heading))>90?QMandala::instance()->current->distance(ne):0;
  if(dist<5)dist=0;
  else if(dist>50000)dist=50000;
  else if(dist>500)dist=(dist/100)*100;
  else if(dist>100)dist=(dist/10)*10;
  f_approach->setValue(dist);
  updatePath();
  emit rwAppPointChanged();
}
double MissionItemRw::heading()
{
  return m_heading;
}
//=============================================================================
void MissionItemRw::updatePath()
{
  QList<QPointF> plist;
  QPointF pt;
  Point llRw(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  //runway line
  Vect llhRw(llRw[0],llRw[1],f_HMSL->value().toDouble());
  plist.append(QPointF(llRw[0],llRw[1]));
  Point ne(f_dN->value().toDouble(),f_dE->value().toDouble());
  Point llB=QMandala::instance()->current->ne2ll(ne,llhRw);
  pt=QPointF(llB[0],llB[1]);
  plist.append(pt);
  m_pathRw.setPath(plist);
  if(m_rwEndPoint!=pt){
    m_rwEndPoint=pt;
    updateHeading();
    emit rwEndPointChanged();
  }
  updateStartPoint();

  //approach straight line
  plist.clear();
  plist.append(QPointF(llRw[0],llRw[1]));
  Point llApp=Mandala::destination(llRw,m_heading+180.0,f_approach->value().toDouble());
  pt=QPointF(llApp[0],llApp[1]);
  plist.append(pt);
  m_pathApp.setPath(plist);
  if(m_rwAppPoint!=pt){
    m_rwAppPoint=pt;
    emit rwAppPointChanged();
  }

  //approach circle
  Point rwNE=QMandala::instance()->current->llh2ne(llhRw);
  double cR=QMandala::instance()->current->turnR;
  bool bLD=QMandala::instance()->current->mode==mode_LANDING && QMandala::instance()->current->rwidx==row();
  if(!bLD){
    cR=f_approach->value().toDouble()/2;
    if(cR<100)cR=100;
    if(f_turn->data(MissionItem::tc_value).toString().toLower().startsWith("left"))cR=-cR;
  }
  if((!bLD)||QMandala::instance()->current->distance(QMandala::instance()->current->cmd_NE-rwNE)<1){
    //default sample circle
    Point llC=Mandala::destination(llApp,m_heading+(cR>=0?90.0:-90.0),fabs(cR));
    m_pathTA.setCircle(QPointF(llC[0],llC[1]),fabs(cR));
  }
  /*double rwHDG=QMandala::instance()->current->heading(modelItem->f_dN->value().toDouble(),modelItem->f_dE->value().toDouble());
  Point cRW=QMandala::instance()->current->llh2ne(Vect(posLL.x(),posLL.y(),QMandala::instance()->current->home_pos[2]));
  Point cP=QMandala::instance()->current->rotate(QMandala::instance()->current->cmd_NE-cRW,rwHDG);
  if((!bLD)||QMandala::instance()->current->distance(QMandala::instance()->current->cmd_NE-cRW)<1) p.addEllipse(0,-modelItem->f_approach->value().toDouble()*sf-r_s,r2,r2);
  else p.addEllipse(0,cP[0]*sf-r_s,r2,r2);
  if(bLD)p.addRect(-100.0*sf,0,sf*200.0,QMandala::instance()->current->delta*sf);*/
}
//=============================================================================
void MissionItemRw::updateHeading()
{
  Point ne(f_dN->value().toDouble(),f_dE->value().toDouble());
  double v=QMandala::instance()->current->boundAngle360(QMandala::instance()->current->heading(ne));
  if(m_heading==v)return;
  m_heading=v;
  emit headingChanged();
}
//=============================================================================
void MissionItemRw::updateStartPoint()
{
  if(row()!=0)return;
  //if(row()!=QMandala::instance()->current->rwidx)return;
  //update mission start point
  double slen=f_approach->value().toDouble()*2.0-QMandala::instance()->current->distance(Point(f_dN->value().toDouble(),f_dE->value().toDouble()));
  model->setStartPointVector(m_rwEndPoint,Mandala::boundAngle(m_heading),slen<0?0:slen);
}
//=============================================================================
