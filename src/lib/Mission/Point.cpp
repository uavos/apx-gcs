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
#include "Point.h"
#include "Mission.h"
#include "MissionItems.h"
#include "VehicleMission.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
//=============================================================================
Point::Point(Points *parent)
  : MissionOrderedItem(parent,"P#","",tr("Point of interest")),
    points(parent)
{
  f_latitude=new Fact(this,"latitude",tr("Latitude"),tr("Global postition latitude"),FactItem,FloatData);
  f_latitude->setUnits("lat");
  f_longitude=new Fact(this,"longitude",tr("Longitude"),tr("Global postition longitude"),FactItem,FloatData);
  f_longitude->setUnits("lon");

  f_hmsl=new Fact(this,"hmsl",tr("HMSL"),tr("Object of interest altitude MSL"),FactItem,IntData);
  f_hmsl->setUnits("m");
  f_hmsl->setEnumStrings(QStringList()<<"ground");


  f_radius=new Fact(this,"radius",tr("Radius"),tr("Loiter radius"),FactItem,IntData);
  f_radius->setUnits("m");
  f_radius->setMin(-10000);
  f_radius->setMax(10000);
  f_radius->setValue(200);

  f_loops=new Fact(this,"loops",tr("Loops"),tr("Loiter loops limit"),FactItem,IntData);
  f_loops->setEnumStrings(QStringList()<<"default");
  f_loops->setMin(0);
  f_loops->setMax(255);

  f_time=new Fact(this,"time",tr("Time"),tr("Loiter time limit"),FactItem,IntData);
  f_time->setEnumStrings(QStringList()<<"default");
  f_time->setUnits("time");
  f_time->setMin(0);
  f_time->setMax(60*60*24);

  //conversions
  connect(f_latitude,&Fact::valueChanged,this,&Point::radiusPointChanged);
  connect(f_longitude,&Fact::valueChanged,this,&Point::radiusPointChanged);
  connect(f_radius,&Fact::valueChanged,this,&Point::radiusPointChanged);

  //title
  connect(f_radius,&Fact::valueChanged,this,&Point::updateTitle);
  connect(f_hmsl,&Fact::valueChanged,this,&Point::updateTitle);
  connect(f_loops,&Fact::valueChanged,this,&Point::updateTitle);
  connect(f_time,&Fact::valueChanged,this,&Point::updateTitle);
  updateTitle();

  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Point::updateTitle()
{
  QStringList st;
  st.append(QString::number(num()+1));
  int r=f_radius->value().toInt();
  if(abs(r)>0){
    st.append(FactSystem::distanceToString(abs(r)));
    if(r<0) st.append(tr("CCW"));
  }else st.append("H");
  if(!f_loops->isZero()) st.append("L"+f_loops->text());
  if(!f_time->isZero()) st.append("T"+f_time->text());
  if(!f_hmsl->isZero()) st.append("MSL"+f_hmsl->text());
  setTitle(st.join(' '));
}
//=============================================================================
QGeoCoordinate Point::radiusPoint() const
{
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  return p.atDistanceAndAzimuth(abs(f_radius->value().toInt()),90.0);
}
void Point::setRadiusPoint(const QGeoCoordinate &v)
{
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  double a=qDegreesToRadians(p.azimuthTo(v));
  double d=p.distanceTo(v);
  QPointF ne(d*cos(a),d*sin(a));
  ne=FactSystem::rotate(ne,90.0);
  int rabs=abs(f_radius->value().toInt());
  if(fabs(ne.y())>(rabs/2.0)){
    //switch turn direction
    f_radius->setValue(ne.y()>0?rabs:-rabs);
  }
  int dist=ne.x();
  if(dist<20)dist=0;
  else if(dist>50000)dist=50000;
  else if(dist>500)dist=(dist/100)*100;
  else dist=(dist/10)*10;
  f_radius->setValue(f_radius->value().toInt()<0?-dist:dist);
}
//=============================================================================
