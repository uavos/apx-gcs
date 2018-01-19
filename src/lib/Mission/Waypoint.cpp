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
#include "Waypoint.h"
#include "Mission.h"
#include "VehicleMission.h"
#include "MissionField.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "Mandala.h"
//=============================================================================
Waypoint::Waypoint(MissionGroup *parent)
  : MissionItem(parent,"W#","",tr("Waypoint")),
    icourse(0),
    m_reachable(false),
    m_warning(false)
{
  f_altitude=new MissionField(this,"altitude",tr("Altitude"),tr("Altitude above ground"),IntData);
  f_altitude->setUnits("m");

  f_type=new MissionField(this,"type",tr("Type"),tr("Maneuver type"),EnumData);
  f_type->setEnumStrings(QMetaEnum::fromType<ManeuverType>());

  //actions
  f_actions=new Fact(this,"actions",tr("Actions"),tr("Actions to perform on waypoint"),GroupItem,NoData);
  f_speed=new MissionField(f_actions,"speed",tr("Speed"),tr("Fly with this speed to waypoint"),IntData);
  f_speed->setEnumStrings(QStringList()<<"cruise");
  f_speed->setUnits("m/s");
  f_speed->setMin(0);
  f_speed->setMax(1000);
  f_shot=new MissionField(f_actions,"shot",tr("Shot"),tr("Make a cam shot on waypoint"),EnumData);
  f_shot->setEnumStrings(QStringList()<<"no"<<"yes");
  f_dshot=new MissionField(f_actions,"dshot",tr("Auto Shot"),tr("Start continuous cam shots on waypoint"),IntData);
  f_dshot->setEnumStrings(QStringList()<<"stop");
  f_dshot->setUnits("m");
  f_script=new MissionField(f_actions,"script",tr("Script"),tr("Execute VM script (@function) on waypoint"),TextData);
  f_poi=new MissionField(f_actions,"poi",tr("POI"),tr("Linked Point Of Interest"),IntData);
  f_poi->setEnumStrings(QStringList()<<"off");
  f_poi->setMin(0);
  f_loiter=new MissionField(f_actions,"loiter",tr("Loiter"),tr("Loiter around POI or waypoint"),EnumData);
  f_loiter->setEnumStrings(QStringList()<<"no"<<"yes");
  f_turnR=new MissionField(f_actions,"turnR",tr("Radius"),tr("Loiter radius"),IntData);
  f_turnR->setEnumStrings(QStringList()<<"default");
  f_turnR->setUnits("m");
  f_loops=new MissionField(f_actions,"loops",tr("Loops"),tr("Loiter loops limit"),IntData);
  f_loops->setEnumStrings(QStringList()<<"default");
  f_loops->setMin(0);
  f_time=new MissionField(f_actions,"time",tr("Time"),tr("Loiter time limit"),IntData);
  f_time->setEnumStrings(QStringList()<<"default");
  f_time->setUnits("time");
  f_time->setMin(0);
  f_time->setMax(60*60*24);

  //default values
  Waypoint *f0=static_cast<Waypoint*>(prevItem());
  if(f0)f_altitude->setValue(f0->f_altitude->value());


  connect(f_actions,&Fact::childValueChanged,this,&Waypoint::updateActionsText);

  connect(f_type,&Fact::valueChanged,this,&Waypoint::updatePath);


  connect(f_type,&Fact::valueChanged,this,&Waypoint::updateTitle);
  connect(f_altitude,&Fact::valueChanged,this,&Waypoint::updateTitle);
  connect(f_actions,&Fact::statusChanged,this,&Waypoint::updateTitle);
  updateTitle();

  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Waypoint::updateTitle()
{
  QStringList st;
  st.append(QString::number(num()+1));
  st.append(f_type->text().left(1).toUpper());
  st.append(f_altitude->text()+f_altitude->units());
  if(!f_actions->isZero()) st.append(f_actions->status());
  setTitle(st.join(' '));
}
/*void Waypoint::updateDescr()
{
  QStringList st;
  st.append(tr("Waypoint")+":");
  st.append(f_type->text());
  st.append(f_altitude->text()+f_altitude->units());
  if(!f_actions->status().isEmpty()) st.append("["+f_actions->status()+"]");
  setDescr(st.join(' '));
}*/
void Waypoint::updateActionsText()
{
  QStringList st;
  for(int i=0;i<f_actions->size();++i){
    Fact *f=static_cast<Fact*>(f_actions->child(i));
    if(f->isZero())continue;
    st.append(f->title());
  }
  f_actions->setStatus(st.join(','));
}
//=============================================================================
QGeoPath Waypoint::getPath()
{
  QGeoPath p;

  VehicleMandala *vm=group->mission->vehicle->f_mandala;
  double spd=0;//QMandala::instance()->current->apcfg.value("spd_cruise").toDouble();
  /*if(f_speed->value().toUInt()>0)
    spd=f_speed->value().toUInt();*/
  if(spd<=0)spd=22;
  double dt=1.0;
  double turnR=0;//QMandala::instance()->current->apcfg.value("turnR").toDouble();
  if(turnR<=0)turnR=100;
  double turnRate=(360.0/(2.0*M_PI))*spd/turnR;
  double crs=m_course;
  double distance=0;
  MissionItem *prev=prevItem();
  QGeoCoordinate dest(coordinate());
  QGeoCoordinate pt;
  bool wptReached=true;
  bool wptWarning=false;
  bool wptLine=false;
  while(1){
    if(!prev){
      pt=group->mission->startPoint();
      if(!pt.isValid()){
        pt=QGeoCoordinate(vm->factById(idx_home_pos|(0<<8))->value().toDouble(),vm->factById(idx_home_pos|(1<<8))->value().toDouble());
        break;
      }
      p.addCoordinate(pt);
      crs=group->mission->startHeading();
      double slen=group->mission->startLength();
      if(slen>0){
        pt=pt.atDistanceAndAzimuth(slen,crs);
        p.addCoordinate(pt);
        distance+=slen;
      }
    }else{
      pt=prev->coordinate();
      if(!prev->geoPath().path().isEmpty()){
        crs=prev->course();
        wptLine=f_type->value().toInt()==Line;
      }else wptLine=true;
    }
    //fly to wpt
    p.addCoordinate(pt);
    //int cnt=0;
    double turnCnt=0;
    while(1){
      double deltaHdg=FactSystem::angle(pt.azimuthTo(dest)-crs);
      double deltaDist=pt.distanceTo(dest);
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
          pt=dest;
          p.addCoordinate(dest);
          wptReached=true;
          break;
        }
      }
      //propagate position
      pt=pt.atDistanceAndAzimuth(step,crs);
      distance+=step;
      deltaHdg=dt*FactSystem::limit(deltaHdg,-turnRate,turnRate);
      crs+=deltaHdg;
      p.addCoordinate(pt);
      turnCnt+=deltaHdg;
      if(fabs(turnCnt)>(360*2)){//(++cnt)>(360/turnRate)){
        wptReached=false;
        break;
      }
    }
    if(p.size()<2)
      p.addCoordinate(dest);
    //qDebug()<<plist;
    break;
  }
  //p.addCoordinate(dest);

  //update properties
  wptWarning|=distance<turnR*(2.0*M_PI*0.8);

  setReachable(wptReached);
  setWarning(wptWarning);

  setDistance(distance);
  setTime(distance/spd);

  //end course
  if(p.size()==2 && crs==m_course){
    crs=p.path().first().azimuthTo(p.path().at(1));
  }
  crs=FactSystem::angle(crs);
  int icrs=(int)(crs/10)*10;
  if(icourse!=icrs){
    icourse=icrs;
    //force next item to be updated
    MissionItem *next=nextItem();
    if(next)next->resetPath();
  }
  setCourse(crs);

  return p;
}
//=============================================================================
//=============================================================================
bool Waypoint::reachable() const
{
  return m_reachable;
}
void Waypoint::setReachable(bool v)
{
  if(m_reachable==v)return;
  m_reachable=v;
  emit reachableChanged();
}
bool Waypoint::warning() const
{
  return m_warning;
}
void Waypoint::setWarning(bool v)
{
  if(m_warning==v)return;
  m_warning=v;
  emit warningChanged();
}
//=============================================================================
//=============================================================================
