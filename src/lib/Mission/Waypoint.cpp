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
#include "MissionItems.h"
#include "VehicleMission.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
#include "Mandala.h"
//=============================================================================
Waypoint::Waypoint(Waypoints *parent)
  : MissionOrderedItem(parent,"W#","",tr("Waypoint")),
    waypoints(parent),
    icourse(0),
    m_course(0),
    m_reachable(false),m_warning(false),
    m_time(0),m_DW(0)
{
  f_altitude=new Fact(this,"altitude",tr("Altitude"),tr("Altitude above ground"),FactItem,IntData);

  f_type=new Fact(this,"type",tr("Type"),tr("Maneuver type"),FactItem,EnumData);
  f_type->setEnumStrings(QMetaEnum::fromType<ManeuverType>());

  f_latitude=new Fact(this,"latitude",tr("Latitude"),tr("Global postition latitude"),FactItem,FloatData);
  f_longitude=new Fact(this,"longitude",tr("Longitude"),tr("Global postition longitude"),FactItem,FloatData);


  connect(f_type,&Fact::valueChanged,this,&Waypoint::updatePath);
  connect(f_latitude,&Fact::valueChanged,this,&Waypoint::updatePath);
  connect(f_longitude,&Fact::valueChanged,this,&Waypoint::updatePath);
  connect(f_altitude,&Fact::valueChanged,this,&Waypoint::updatePath);

  //connect(this,&Fact::nameChanged,this,&Waypoint::updatePath);
  connect(this,&Fact::titleChanged,this,&Waypoint::updatePath,Qt::QueuedConnection);

  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Waypoint::updatePath()
{
  VehicleMandala *vm=missionItems->mission->vehicle->f_mandala;
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
  //QList<QGeoCoordinate> plist;
  plist.clear();
  Waypoint *prevWp=prevItem<Waypoint*>();
  QGeoCoordinate dest(f_latitude->value().toDouble(),f_longitude->value().toDouble(),f_altitude->value().toDouble());
  QGeoCoordinate pt;
  bool wptReached=true;
  bool wptWarning=false;
  bool wptLine=false;
  while(1){
    if(!prevWp){
      //pt=model->startPoint();
      if(!pt.isValid()){
        pt=QGeoCoordinate(vm->factById(idx_home_pos|(0<<8))->value().toDouble(),vm->factById(idx_home_pos|(1<<8))->value().toDouble());
        break;
      }
      plist.append(pt);
      crs=0;//model->startCourse();
      double slen=0;//model->startLength();
      if(slen>0){
        pt=pt.atDistanceAndAzimuth(slen,crs);
        plist.append(pt);
        distance+=slen;
      }
    }else{
      pt=QGeoCoordinate(prevWp->f_latitude->value().toDouble(),prevWp->f_longitude->value().toDouble(),prevWp->f_altitude->value().toDouble());
      if(!prevWp->flightPath().path().isEmpty()){
        crs=prevWp->course();
        wptLine=f_type->value().toInt()==Line;
      }else wptLine=true;
    }
    //fly to wpt
    QGeoCoordinate ll_dest(dest);

    //loop
    plist.append(pt);
    //int cnt=0;
    double turnCnt=0;
    while(1){
      double deltaHdg=FactSystem::angle(pt.azimuthTo(ll_dest)-crs);
      double deltaDist=pt.distanceTo(ll_dest);
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
          pt=ll_dest;
          plist.append(dest);
          wptReached=true;
          break;
        }
      }
      //propagate position
      pt=pt.atDistanceAndAzimuth(step,crs);
      distance+=step;
      deltaHdg=dt*FactSystem::limit(deltaHdg,-turnRate,turnRate);
      crs+=deltaHdg;
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
    crs=plist.at(0).azimuthTo(plist.at(1));
  }
  //update path
  setReachable(wptReached);
  setWarning(wptWarning);
  if(m_flightPath.path()!=plist){
    m_flightPath.setPath(plist);
    emit flightPathChanged();
  }
  //qDebug()<<plist.size();

  crs=FactSystem::angle(crs);
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
  Waypoint *nextWp=nextItem<Waypoint*>();
  if(nextWp){
    doUpdNext|=nextWp->flightPath().path().size()<2 || nextWp->flightPath().path().first()!=dest;
    if(doUpdNext)nextWp->updatePath();
  }
}
//=============================================================================
//=============================================================================
QGeoPath Waypoint::flightPath() const
{
  return m_flightPath;
}
double Waypoint::course() const
{
  return m_course;
}
void Waypoint::setCourse(const double &v)
{
  if(m_course==v)return;
  m_course=v;
  emit courseChanged();
}
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
uint Waypoint::time() const
{
  return m_time;
}
uint Waypoint::DW() const
{
  return m_DW;
}
uint Waypoint::DT() const
{
  int v=0;
  const Waypoint *i=this;
  do{
    v+=i->DW();
    i=i->prevItem<Waypoint*>();
  }while(i);
  return v;
}
uint Waypoint::ETA() const
{
  int v=0;
  const Waypoint *i=this;
  do{
    v+=i->time();
    i=i->prevItem<Waypoint*>();
  }while(i);
  return v;
}
//=============================================================================
//=============================================================================
