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
#include "MissionItemWp.h"
#include "MissionItemField.h"
#include "QMandala.h"
//=============================================================================
MissionItemWp::MissionItemWp(MissionItemCategory<MissionItemWp> *parent)
 : MissionItemObject(parent,parent->model,parent->childName),
   icourse(0),
   m_reachable(false),m_warning(false),
   m_course(0),m_time(0),m_DW(0)
{
  f_altitude=new MissionItemField(this,"altitude",MissionItemField::dt_distance,QStringList(),tr("Waypoint altitude"));
  f_type=new MissionItemField(this,"type",MissionItemField::dt_option,QStringList()<<"hdg"<<"line",tr("Maneuver type"));
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat,QStringList(),tr("Waypoint global position (latitude)"));
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon,QStringList(),tr("Waypoint global position (longitude)"));

  f_actions=new MissionItemFieldGroup(this,"actions",QString(),tr("Actions to perform on waypoint"));
  f_speed=new MissionItemField(f_actions,"speed",MissionItemField::dt_byte,QStringList()<<"cruise",tr("Fly with this speed to waypoint"));
  f_shot=new MissionItemField(f_actions,"shot",MissionItemField::dt_option,QStringList()<<"no"<<"yes",tr("Make a cam shot on waypoint"));
  f_scr=new MissionItemField(f_actions,"script",MissionItemField::dt_script,QStringList(),tr("Execute VM script (@function) on waypoint"));
  f_poi=new MissionItemField(f_actions,"POI",MissionItemField::dt_byte,QStringList()<<"off",tr("Linked POI"));
  f_loiter=new MissionItemField(f_actions,"loiter",MissionItemField::dt_option,QStringList()<<"no"<<"yes",tr("Loiter around POI or waypoint"));
  f_turnR=new MissionItemField(f_actions,"turnR",MissionItemField::dt_distance,QStringList()<<"default",tr("Loiter radius"));
  f_loops=new MissionItemField(f_actions,"loops",MissionItemField::dt_byte,QStringList()<<"default",tr("Loiter loops limit"));
  f_time=new MissionItemField(f_actions,"time",MissionItemField::dt_time,QStringList()<<"default",tr("Loiter time limit"));

  //default values
  QPointF ll(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());
  if(prevItem()) f_altitude->setValue(prevItem()->f_altitude->value());
  else{
    double alt=QMandala::instance()->current->apcfg.value("altitude").toDouble();
    if(alt==0)alt=100;
    f_altitude->setValue(alt);
  }

  connect(model,SIGNAL(addedRemoved()),this,SLOT(updatePath()),Qt::QueuedConnection);

  connect(f_latitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_longitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_type,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_altitude,SIGNAL(changed()),this,SLOT(updatePath()));

  connect(f_actions,SIGNAL(changed()),this,SLOT(updatePath()));

  connect(QMandala::instance()->current,SIGNAL(apcfgChanged()),this,SLOT(updatePath()),Qt::QueuedConnection);

  connect(parent,SIGNAL(changed()),this,SIGNAL(dtChanged()));
  connect(parent,SIGNAL(changed()),this,SIGNAL(etaChanged()));

  connect(model,SIGNAL(startPointChanged()),this,SLOT(updateStartPath()));
  updatePath();
}
//=============================================================================
QVariant MissionItemWp::data(int column,int role) const
{
  if(column!=tc_field || role!=Qt::DisplayRole) return MissionItem::data(column,role);
  QStringList st;
  st.append("#"+QString::number(row()+1));
  st.append(f_type->data(tc_value).toString().at(0).toUpper());
  st.append(f_altitude->value().toString()+"m");
  if(!f_actions->isZero())
    st.append("A");
  return st.join(' ');
  //return MissionItem::name()+QString::number(row()+1);
}
//=============================================================================
QVariant MissionItemWp::value(void) const
{
  QStringList st;
  st.append(QMandala::distanceToString(DT()));
  double timeT=ETA();
  qint64 d=(qint64)timeT/(24*60*60);
  if(d<=0)st.append(QString("%1").arg(QTime(0,0,0).addSecs(timeT).toString("hh:mm")));
  else st.append(QString("%1d%2").arg(d).arg(QTime(0,0,0).addSecs(timeT).toString("hh:mm")));
  st.append(descr());
  st.removeAll("");
  return st.join(' ');
}
//=============================================================================
QStringList MissionItemWp::getToolTip(void) const
{
  QStringList st=MissionItem::getToolTip();
  QString s=st.first();
  st.clear();
  st.append(s);
  st.append(QString("%1: %2 km").arg("DH").arg(QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(f_latitude->value().toDouble(),f_longitude->value().toDouble(),f_altitude->value().toDouble()+QMandala::instance()->current->home_pos[2])))/1000.0,0,'f',1));
  st.append(QString("%1: %2 km").arg("DW").arg(DW()/1000.0,0,'f',1));
  st.append(QString("%1: %2 km").arg("DT").arg(DT()/1000.0,0,'f',1));
  double timeT=ETA();
  qint64 d=(qint64)timeT/(24*60*60);
  if(d<=0)st.append(QString("%1: %2").arg("ETA").arg(QTime(0,0,0).addSecs(timeT).toString("hh:mm:ss")));
  else st.append(QString("%1: %2d%3").arg("ETA").arg(d).arg(QTime(0,0,0).addSecs(timeT).toString("hh:mm:ss")));
  if(!f_actions->isZero()){
    st.append("");
    st.append(f_actions->getToolTip());
  }
  return st;
}
//=============================================================================
uint MissionItemWp::ETA() const
{
  int v=0;
  const MissionItemWp *i=this;
  do{
    v+=i->time();
    i=i->prevItem();
  }while(i);
  return v;
}
uint MissionItemWp::DT() const
{
  int v=0;
  const MissionItemWp *i=this;
  do{
    v+=i->DW();
    i=i->prevItem();
  }while(i);
  return v;
}
uint MissionItemWp::DW() const
{
  return m_DW;
}
double MissionItemWp::course()
{
  return m_course;
}
uint MissionItemWp::time() const
{
  return m_time;
}
//=============================================================================
MissionItemWp *MissionItemWp::prevItem() const
{
  if(row()==0) return NULL;
  return static_cast<MissionItemWp *>(parentItem->child(row()-1));
}
MissionItemWp *MissionItemWp::nextItem() const
{
  if(row()>=(parentItem->childCount()-1)) return NULL;
  return static_cast<MissionItemWp *>(parentItem->child(row()+1));
}
//=============================================================================
MissionPath * MissionItemWp::path()
{
  return &m_path;
}
bool MissionItemWp::reachable()
{
  return m_reachable;
}
void MissionItemWp::setReachable(bool v)
{
  if(m_reachable==v)return;
  m_reachable=v;
  emit reachableChanged();
}
bool MissionItemWp::warning()
{
  return m_warning;
}
void MissionItemWp::setWarning(bool v)
{
  if(m_warning==v)return;
  m_warning=v;
  emit warningChanged();
}
//=============================================================================
void MissionItemWp::updatePath()
{
  double spd=QMandala::instance()->current->apcfg.value("spd_cruise").toDouble();
  if(f_speed->value().toUInt()>0)
    spd=f_speed->value().toUInt();
  if(spd<=0)spd=22;
  double dt=1.0;
  double turnR=QMandala::instance()->current->apcfg.value("turnR").toDouble();
  if(turnR<=0)turnR=100;
  double turnRate=(360.0/(2.0*M_PI))*spd/turnR;
  double crs=m_course;
  double distance=0;
  QList<QPointF> plist;
  MissionItemWp *prevWp=prevItem();
  QPointF dest(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  QPointF pt;
  bool wptReached=true;
  bool wptWarning=false;
  bool wptLine=false;
  while(1){
    if(!prevWp){
      pt=model->startPoint();
      if(pt.isNull()){
        pt=QPointF(QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]));
        break;
      }
      plist.append(pt);
      crs=model->startCourse();
      double slen=model->startLength();
      if(slen>0){
        Point ll=Mandala::destination(Point(pt.x(),pt.y()),crs,slen);
        pt=QPointF(ll[0],ll[1]);
        plist.append(pt);
        distance+=slen;
      }
    }else{
      pt=QPointF(prevWp->f_latitude->value().toDouble(),prevWp->f_longitude->value().toDouble());
      if(prevWp->path()->path().size()){
        crs=prevWp->course();
        wptLine=f_type->data(tc_value).toString().toLower()=="line";
      }else wptLine=true;
    }
    //fly to wpt
    Point ll_dest(dest.x(),dest.y());
    Point ll(pt.x(),pt.y());

    //loop
    plist.append(pt);
    //int cnt=0;
    double turnCnt=0;
    while(1){
      double deltaHdg=QMandala::instance()->current->boundAngle(QMandala::instance()->current->bearing(ll,ll_dest)-crs);
      double deltaDist=QMandala::instance()->current->distance(ll,ll_dest);
      double step=dt*spd;
      if(wptLine || fabs(deltaHdg)<(dt*10.0)){
        //crs ok (turn finished)
        step=10.0e+3*dt;
        crs+=deltaHdg;
        deltaHdg=0;
        if(deltaDist<=step){
          //wpt reached
          crs+=deltaHdg;
          deltaHdg=0;
          distance+=deltaDist;
          ll=ll_dest;
          plist.append(dest);
          wptReached=true;
          break;
        }
      }
      //propagate position
      ll=QMandala::instance()->current->destination(ll,crs,step);
      distance+=step;
      deltaHdg=dt*QMandala::instance()->current->limit(deltaHdg,-turnRate,turnRate);
      crs+=deltaHdg;
      pt=QPointF(ll[0],ll[1]);
      plist.append(pt);
      turnCnt+=deltaHdg;
      if(fabs(turnCnt)>(360*2)){//(++cnt)>(360/turnRate)){
        wptReached=false;
        break;
      }
    }
    if(plist.size()<2)
      plist.append(dest);
    //qDebug()<<plist;
    break;
  }

  if(plist.size()<2){
    //plist.append(pt);
    //plist.append(dest);
    //distance=Mandala::distance(Point(pt.x(),pt.y()),Point(dest.x(),dest.y()));
  }

  //calc additional information and updates
  bool doUpdNext=false;
  wptWarning|=distance<turnR*(2.0*M_PI*0.8);
  uint idist=distance;
  if(m_DW!=idist){
    m_DW=idist;
    emit dwChanged();
  }
  uint itime=distance/spd;
  if(m_time!=itime){
    m_time=itime;
    emit timeChanged();
  }
  //end course
  if(plist.size()==2 && crs==m_course){
    Point ll1(plist.at(plist.size()-2).x(),plist.at(plist.size()-2).y());
    Point ll2(plist.at(plist.size()-1).x(),plist.at(plist.size()-1).y());
    crs=QMandala::instance()->current->bearing(ll1,ll2);
  }
  //update path
  setReachable(wptReached);
  setWarning(wptWarning);
  m_path.setPath(plist);

  crs=QMandala::instance()->current->boundAngle(crs);
  int icrs=(int)(crs/10)*10;
  if(icourse!=icrs){
    icourse=icrs;
    doUpdNext=true;
  }
  if(m_course!=crs){
    m_course=crs;
    emit courseChanged();
  }
  //check to propagate updates to next wpts
  MissionItemWp *nextWp=nextItem();
  if(nextWp){
    doUpdNext|=nextWp->path()->path().size()<2 || nextWp->path()->path().first()!=dest;
    if(doUpdNext)nextWp->updatePath();
  }
}
//=============================================================================
void MissionItemWp::updateStartPath()
{
  if(row()==0) updatePath();
}
//=============================================================================
QByteArray MissionItemWp::pack() const
{
  Mission::_item_wp v;
  v.hdr.type=Mission::mi_wp;
  v.hdr.option=f_type->value().toUInt();
  v.lat=f_latitude->value().toFloat();
  v.lon=f_longitude->value().toFloat();
  v.alt=f_altitude->value().toInt();
  QByteArray ba((const char*)&v,sizeof(v));
  if(f_actions->isZero())return ba;
  if(!f_speed->isZero()){
    Mission::_item_action a;
    a.hdr.type=Mission::mi_action;
    a.hdr.option=Mission::mo_speed;
    a.speed=f_speed->value().toUInt();
    ba.append((const char*)&a,Mission::action_size(a.hdr.option));
  }
  if(!f_poi->isZero()){
    Mission::_item_action a;
    a.hdr.type=Mission::mi_action;
    a.hdr.option=Mission::mo_poi;
    a.poi=f_poi->value().toUInt()-1;
    ba.append((const char*)&a,Mission::action_size(a.hdr.option));
  }
  if(!f_scr->isZero()){
    Mission::_item_action a;
    a.hdr.type=Mission::mi_action;
    a.hdr.option=Mission::mo_scr;
    strncpy(a.scr,f_scr->value().toString().toUtf8().data(),sizeof(Mission::_item_action::scr));
    ba.append((const char*)&a,Mission::action_size(a.hdr.option));
  }
  if(!f_loiter->isZero()){
    Mission::_item_action a;
    a.hdr.type=Mission::mi_action;
    a.hdr.option=Mission::mo_loiter;
    a.loiter.turnR=f_turnR->value().toInt();
    a.loiter.loops=f_loops->value().toUInt();
    a.loiter.timeS=f_time->value().toUInt();
    ba.append((const char*)&a,Mission::action_size(a.hdr.option));
  }
  if(!f_shot->isZero()){
    Mission::_item_action a;
    a.hdr.type=Mission::mi_action;
    a.hdr.option=Mission::mo_shot;
    ba.append((const char*)&a,Mission::action_size(a.hdr.option));
  }
  return ba;
}
int MissionItemWp::unpack(const QByteArray &ba)
{
  if(ba.size()<(int)sizeof(Mission::_item_wp))return 0;
  Mission::_item_wp *v=(Mission::_item_wp*)ba.data();
  if(v->hdr.type!=Mission::mi_wp)return 0;
  f_type->setValue((uint)v->hdr.option);
  f_latitude->setValue(v->lat);
  f_longitude->setValue(v->lon);
  f_altitude->setValue(v->alt);
  int cnt=sizeof(Mission::_item_wp);
  while(ba.mid(cnt).size()>=(int)sizeof(Mission::_item_hdr) && ((Mission::_item_hdr*)ba.data()+cnt)->type==Mission::mi_action){
    Mission::_item_action *v=(Mission::_item_action*)(ba.data()+cnt);
    int sz=Mission::action_size(v->hdr.option);
    if((ba.size()-cnt)<sz)return 0; //error
    cnt+=sz;
    switch(v->hdr.option){
      case Mission::mo_speed:   f_speed->setValue(v->speed);break;
      case Mission::mo_poi:     f_poi->setValue(v->poi+1);break;
      case Mission::mo_scr:     f_scr->setValue(QString(v->scr));break;
      case Mission::mo_loiter:
        f_loiter->setValue(1);
        f_turnR->setValue(v->loiter.turnR);
        f_loops->setValue(v->loiter.loops);
        f_time->setValue(v->loiter.timeS);
      break;
      case Mission::mo_shot:    f_shot->setValue(1);break;
      default: return 0;
    }
  }
  return cnt;
}
//=============================================================================

