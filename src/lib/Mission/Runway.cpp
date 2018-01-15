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
#include "Runway.h"
#include "Mission.h"
#include "MissionItems.h"
#include "VehicleMission.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
//=============================================================================
Runway::Runway(Runways *parent)
  : MissionOrderedItem(parent,"R#","",""),
    runways(parent)
{
  f_type=new Fact(this,"type",tr("Type"),tr("Landing pattern type"),FactItem,EnumData);
  f_type->setEnumStrings(QMetaEnum::fromType<RunwayType>());

  f_approach=new Fact(this,"approach",tr("Approach length"),tr("Final approach length"),FactItem,IntData);
  f_approach->setUnits("m");
  f_approach->setMin(0);
  f_approach->setMax(10000);

  f_hmsl=new Fact(this,"hmsl",tr("HMSL"),tr("Runway altitude above mean sea level"),FactItem,IntData);
  f_hmsl->setUnits("m");
  f_hmsl->setEnumStrings(QStringList()<<"default");

  f_latitude=new Fact(this,"latitude",tr("Latitude"),tr("Global postition latitude"),FactItem,FloatData);
  f_latitude->setUnits("lat");
  f_longitude=new Fact(this,"longitude",tr("Longitude"),tr("Global postition longitude"),FactItem,FloatData);
  f_longitude->setUnits("lon");

  f_dN=new Fact(this,"dN",tr("Delta North"),tr("Runway direction point (north)"),FactItem,IntData);
  f_dN->setUnits("m");
  f_dN->setValue(100);
  f_dN->setMin(-10000);
  f_dN->setMax(10000);
  f_dE=new Fact(this,"dE",tr("Delta East"),tr("Runway direction point (east)"),FactItem,IntData);
  f_dE->setUnits("m");
  f_dE->setValue(300);
  f_dE->setMin(-10000);
  f_dE->setMax(10000);


  //conversions
  connect(f_dN,&Fact::valueChanged,this,&Runway::endPointChanged);
  connect(f_dE,&Fact::valueChanged,this,&Runway::endPointChanged);
  connect(f_latitude,&Fact::valueChanged,this,&Runway::endPointChanged);
  connect(f_longitude,&Fact::valueChanged,this,&Runway::endPointChanged);

  connect(f_dN,&Fact::valueChanged,this,&Runway::headingChanged);
  connect(f_dE,&Fact::valueChanged,this,&Runway::headingChanged);

  connect(f_latitude,&Fact::valueChanged,this,&Runway::appPointChanged);
  connect(f_longitude,&Fact::valueChanged,this,&Runway::appPointChanged);
  connect(f_approach,&Fact::valueChanged,this,&Runway::appPointChanged);
  connect(this,&Runway::headingChanged,this,&Runway::appPointChanged);

  connect(f_type,&Fact::valueChanged,this,&Runway::updateDescr);
  connect(f_approach,&Fact::valueChanged,this,&Runway::updateDescr);
  connect(f_hmsl,&Fact::valueChanged,this,&Runway::updateDescr);
  connect(this,&Runway::headingChanged,this,&Runway::updateDescr);

  connect(this,&Runway::endPointChanged,this,&Runway::updateMissionStartPoint);
  connect(this,&Runway::appPointChanged,this,&Runway::updateMissionStartPoint);


  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Runway::updateDescr()
{
  QStringList st;
  st.append(tr("Runway")+":");
  st.append(f_type->text().left(1).toUpper()+QString::number((int)floor(FactSystem::angle360(heading()))));
  st.append(f_approach->text()+f_approach->units());
  if(!f_hmsl->isZero()) st.append("HMSL: "+f_hmsl->text());
  setDescr(st.join(' '));
}
//=============================================================================
void Runway::updateMissionStartPoint()
{
  if(num()!=0)return;
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  QGeoCoordinate p2=endPoint();
  runways->mission->setStartPoint(p2);
  runways->mission->setStartHeading(heading());
  double slen=f_approach->value().toDouble()*2.0-p.distanceTo(endPoint());
  runways->mission->setStartLength(slen<0?0:slen);
}
//=============================================================================
QGeoCoordinate Runway::endPoint() const
{
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  double dN=f_dN->value().toDouble();
  double dE=f_dE->value().toDouble();
  double azimuth=qRadiansToDegrees(atan2(dE,dN));
  double distance=sqrt(pow(dN,2)+pow(dE,2));
  return p.atDistanceAndAzimuth(distance,azimuth);
}
void Runway::setEndPoint(const QGeoCoordinate &v)
{
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  double a=qDegreesToRadians(p.azimuthTo(v));
  double d=p.distanceTo(v);
  f_dN->setValue(d*cos(a));
  f_dE->setValue(d*sin(a));
}
QGeoCoordinate Runway::appPoint() const
{
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  double dN=f_dN->value().toDouble();
  double dE=f_dE->value().toDouble();
  double azimuth=qRadiansToDegrees(atan2(dE,dN))+180.0;
  double distance=f_approach->value().toDouble();
  return p.atDistanceAndAzimuth(distance,azimuth);
}
void Runway::setAppPoint(const QGeoCoordinate &v)
{
  QGeoCoordinate p(f_latitude->value().toDouble(),f_longitude->value().toDouble());
  double a=qDegreesToRadians(p.azimuthTo(v));
  double d=p.distanceTo(v);
  QPointF ne(d*cos(a),d*sin(a));
  ne=FactSystem::rotate(ne,heading()+180.0);
  if(fabs(ne.y())>(f_approach->value().toDouble()/2.0)){
    //switch turn direction
    f_type->setValue(ne.y()>0?Left:Right);
  }
  int dist=ne.x();
  if(dist<5)dist=0;
  else if(dist>50000)dist=50000;
  else if(dist>500)dist=(dist/100)*100;
  else if(dist>100)dist=(dist/10)*10;
  f_approach->setValue(dist);
}
double Runway::heading() const
{
  double dN=f_dN->value().toDouble();
  double dE=f_dE->value().toDouble();
  return qRadiansToDegrees(atan2(dE,dN));
}
//=============================================================================
