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
#include "ItemWpt.h"
#include "MapView.h"
//=============================================================================
MissionItemWp::MissionItemWp(MissionItemCategory<MissionItemWp> *parent)
 : MissionItemObject(parent,parent->model,parent->childName)
{
  f_altitude=new MissionItemField(this,"altitude",MissionItemField::dt_distance,QStringList(),tr("Waypoint altitude"));
  f_type=new MissionItemField(this,"type",MissionItemField::dt_option,QStringList()<<"hdg"<<"line",tr("Maneuver type"));
  f_latitude=new MissionItemField(this,"latitude",MissionItemField::dt_lat,QStringList(),tr("Waypoint global position (latitude)"));
  f_longitude=new MissionItemField(this,"longitude",MissionItemField::dt_lon,QStringList(),tr("Waypoint global position (longitude)"));

  f_actions=new MissionItemFieldGroup(this,"actions",QString(),tr("Actions to perform on waypoint"));
  f_speed=new MissionItemField(f_actions,"speed",MissionItemField::dt_byte,QStringList()<<"cruise",tr("Fly with this speed to waypoint"));
  f_shot=new MissionItemField(f_actions,"shot",MissionItemField::dt_option,QStringList()<<"no"<<"single"<<"dshot",tr("Make a cam shot on waypoint"));
  f_dshot=new MissionItemField(f_actions,"dshot",MissionItemField::dt_distance,QStringList()<<"stop",tr("Start continuous cam shots on waypoint"));
  f_scr=new MissionItemField(f_actions,"script",MissionItemField::dt_script,QStringList(),tr("Execute VM script (@function) on waypoint"));
  f_poi=new MissionItemField(f_actions,"POI",MissionItemField::dt_byte,QStringList()<<"off",tr("Linked POI"));
  f_loiter=new MissionItemField(f_actions,"loiter",MissionItemField::dt_option,QStringList()<<"no"<<"yes",tr("Loiter around POI or waypoint"));
  f_turnR=new MissionItemField(f_actions,"turnR",MissionItemField::dt_distance,QStringList()<<"default",tr("Loiter radius"));
  f_loops=new MissionItemField(f_actions,"loops",MissionItemField::dt_byte,QStringList()<<"default",tr("Loiter loops limit"));
  f_time=new MissionItemField(f_actions,"time",MissionItemField::dt_time,QStringList()<<"default",tr("Loiter time limit"));

  //default values
  QPointF ll(model->mapView->clkLL);
  if(ll.isNull()) ll=QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]);
  f_latitude->setValue(ll.x());
  f_longitude->setValue(ll.y());
  if(prevItem()) f_altitude->setValue(prevItem()->f_altitude->value());
  else{
    double alt=QMandala::instance()->current->apcfg.value("altitude").toDouble();
    if(alt==0)alt=100;
    f_altitude->setValue(alt);
  }

  connect(model,SIGNAL(addedRemoved()),this,SLOT(updatePath()),Qt::QueuedConnection);

  mapItem=new ItemWpt(this);

  connect(f_latitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_longitude,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_type,SIGNAL(changed()),this,SLOT(updatePath()));
  connect(f_altitude,SIGNAL(changed()),this,SLOT(updatePath()));

  connect(f_actions,SIGNAL(changed()),this,SLOT(updatePath()));

  connect(QMandala::instance()->current,SIGNAL(apcfgChanged()),this,SLOT(updatePath()));

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
  st.append(getDistanceString(DME()));
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
  st.append(QString("%1: %2 km").arg(tr("DH")).arg(QMandala::instance()->current->distance(QMandala::instance()->current->llh2ne(Vect(f_latitude->value().toDouble(),f_longitude->value().toDouble(),f_altitude->value().toDouble()+QMandala::instance()->current->home_pos[2])))/1000.0,0,'f',1));
  st.append(QString("%1: %2 km").arg(tr("DW")).arg(distance/1000.0,0,'f',1));
  double distanceT=DME(),timeT=ETA();
  st.append(QString("%1: %2 km").arg(tr("DT")).arg(distanceT/1000.0,0,'f',1));
  qint64 d=(qint64)timeT/(24*60*60);
  if(d<=0)st.append(QString("%1: %2").arg(tr("ETA")).arg(QTime(0,0,0).addSecs(timeT).toString("hh:mm:ss")));
  else st.append(QString("%1: %2d%3").arg(tr("ETA")).arg(d).arg(QTime(0,0,0).addSecs(timeT).toString("hh:mm:ss")));
  if(!f_actions->isZero()){
    st.append("");
    st.append(f_actions->getToolTip());
  }
  return st;
}
//=============================================================================
double MissionItemWp::ETA() const
{
  double v=0;
  const MissionItemWp *i=this;
  do{
    v+=i->time;
    i=i->prevItem();
  }while(i);
  return v;
}
double MissionItemWp::DME() const
{
  double v=0;
  const MissionItemWp *i=this;
  do{
    v+=i->distance;
    i=i->prevItem();
  }while(i);
  return v;
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
  double altitude=f_altitude->value().toDouble();
  double hmsl=altitude+QMandala::instance()->current->home_pos[2];
  double crs=course;
  distance=time=0;
  QList<QPointF> plist;
  MissionItemWp *prevWp=prevItem();
  QPointF dest(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  QPointF pt;
  wptReached=true;
  wptWarning=false;
  while(1){
    if(!prevWp){
      pt=QPointF(QPointF(QMandala::instance()->current->home_pos[0],QMandala::instance()->current->home_pos[1]));
      break;
    }
    pt=QPointF(prevWp->f_latitude->value().toDouble(),prevWp->f_longitude->value().toDouble());
    if(f_type->data(tc_value).toString().toLower()=="line")
      break;
    crs=prevWp->course;
    //fly to wpt
    Vect llh_dest(dest.x(),dest.y(),hmsl);
    Vect llh(pt.x(),pt.y(),hmsl);
    Point wptNE=QMandala::instance()->current->llh2ne(llh_dest,llh);
    Point ne;//=QMandala::instance()->current->llh2ne(llh,Vect());

    bool bQuickJump=true;//pt.isNull()||dest.isNull();
    //loop
    plist.append(pt);
    //int cnt=0;
    double turnCnt=0;
    while(1){
      Point dNE=wptNE-ne;
      double deltaHdg=QMandala::instance()->current->boundAngle(QMandala::instance()->current->heading(dNE)-crs);
      double deltaDist=QMandala::instance()->current->distance(wptNE-ne);
      if(bQuickJump && fabs(deltaHdg)<(dt*10.0)){
        //wpt reached
        crs+=deltaHdg;
        deltaHdg=0;
        distance+=deltaDist;
        ne=wptNE;
        plist.append(dest);
        wptReached=true;
        break;
      }
      if((!bQuickJump) && deltaDist<=dt*spd){
        distance+=deltaDist;
        ne=wptNE;
        plist.append(dest);
        wptReached=true;
        break;
      }
      //propagate position
      double crs_r=crs*D2R;
      dNE=dt*spd*Point(cos(crs_r),sin(crs_r));
      double delta=QMandala::instance()->current->distance(dNE);
      distance+=delta;
      ne+=dNE;
      Point ll=QMandala::instance()->current->ne2ll(ne,llh);
      deltaHdg=dt*QMandala::instance()->current->limit(deltaHdg,-turnRate,turnRate);;
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
    plist.append(pt);
    plist.append(dest);
    distance=QMandala::instance()->current->distance(QMandala::instance()->current->LLH_dist(Vect(pt.x(),pt.y(),altitude),Vect(dest.x(),dest.y(),altitude),dest.x(),dest.y()));
  }

  //calc additional information and updates
  bool doUpdNext=false;
  wptWarning|=distance<turnR*(2.0*M_PI*0.8);
  time=distance/spd;
  //end course
  if(plist.size()==2 && crs==course){
    QPointF ptc(plist.at(plist.size()-1)-plist.at(plist.size()-2));
    crs=QMandala::instance()->current->heading(ptc.x(),ptc.y());
  }
  crs=(int)(QMandala::instance()->current->boundAngle(crs)/10)*10;
  if(course!=crs){
    course=crs;
    doUpdNext=true;
  }
  //update path signals
  if(path!=plist){
    path=plist;
    emit pathChanged();
  }
  //check to propagate updates to next wpts
  MissionItemWp *nextWp=nextItem();
  if(nextWp){
    doUpdNext|=nextWp->path.size()<2 || nextWp->path.first()!=dest;
    if(doUpdNext)nextWp->updatePath();
  }
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
    switch(f_shot->value().toUInt()){
      case 1:
      default:
        a.shot.opt=0;
        a.shot.dist=0;
      break;
      case 2:
        if(f_dshot->value().toInt()>0){
          a.shot.opt=1;
          uint v=f_dshot->value().toUInt();
          a.shot.dist=(v>((1<<12)-1))?(1<<12)-1:v;
        }else{
          a.shot.opt=2;
          a.shot.dist=0;
        }
      break;
    }
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
      case Mission::mo_shot:{
        switch(v->shot.opt){
          case 0: //single
            f_shot->setValue(1);
            f_dshot->setValue(0);
          break;
          case 1: //start
            f_shot->setValue(2);
            f_dshot->setValue(v->shot.dist);
          break;
          case 2: //stop
            f_shot->setValue(2);
            f_dshot->setValue(0);
          break;
        }
      }break;
      default: return 0;
    }
  }
  return cnt;
}
//=============================================================================

