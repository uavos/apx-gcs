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
#include "MissionField.h"
#include "VehicleMission.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
//=============================================================================
Runway::Runway(MissionGroup *parent)
  : MissionItem(parent,"R#","",tr("Runway"))
{
  f_type=new MissionField(this,"type",tr("Type"),tr("Landing pattern type"),EnumData);
  f_type->setEnumStrings(QMetaEnum::fromType<RunwayType>());

  f_approach=new MissionField(this,"approach",tr("Approach length"),tr("Final approach length"),IntData);
  f_approach->setUnits("m");
  f_approach->setMin(0);
  f_approach->setMax(10000);

  f_hmsl=new MissionField(this,"hmsl",tr("HMSL"),tr("Runway altitude above mean sea level"),IntData);
  f_hmsl->setUnits("m");
  f_hmsl->setEnumStrings(QStringList()<<"default");

  f_dN=new MissionField(this,"dN",tr("Delta North"),tr("Runway direction point (north)"),IntData);
  f_dN->setUnits("m");
  f_dN->setMin(-10000);
  f_dN->setMax(10000);
  f_dE=new MissionField(this,"dE",tr("Delta East"),tr("Runway direction point (east)"),IntData);
  f_dE->setUnits("m");
  f_dE->setMin(-10000);
  f_dE->setMax(10000);

  //default values
  f_approach->setValue(800);
  f_dN->setValue(100);
  f_dE->setValue(300);

  //title
  connect(f_type,&Fact::valueChanged,this,&Runway::updateTitle);
  connect(f_approach,&Fact::valueChanged,this,&Runway::updateTitle);
  connect(f_hmsl,&Fact::valueChanged,this,&Runway::updateTitle);
  connect(this,&Runway::headingChanged,this,&Runway::updateTitle);
  updateTitle();



  //conversions
  connect(f_dN,&Fact::valueChanged,this,&Runway::endPointChanged);
  connect(f_dE,&Fact::valueChanged,this,&Runway::endPointChanged);
  connect(this,&MissionItem::coordinateChanged,this,&Runway::endPointChanged);

  connect(f_dN,&Fact::valueChanged,this,&Runway::headingChanged);
  connect(f_dE,&Fact::valueChanged,this,&Runway::headingChanged);

  connect(this,&MissionItem::coordinateChanged,this,&Runway::appPointChanged);
  connect(f_approach,&Fact::valueChanged,this,&Runway::appPointChanged);
  connect(this,&Runway::headingChanged,this,&Runway::appPointChanged);

  connect(this,&Runway::endPointChanged,this,&Runway::updateMissionStartPoint);
  connect(this,&Runway::appPointChanged,this,&Runway::updateMissionStartPoint);


  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Runway::updateTitle()
{
  QStringList st;
  st.append(QString::number(num()+1));
  st.append(f_type->text().left(1).toUpper()+QString("%1").arg((int)FactSystem::angle360(round(FactSystem::angle360(heading())/10.0)*10)/10,2,10,QLatin1Char('0')));
  st.append(FactSystem::distanceToString(f_approach->value().toInt()));
  if(!f_hmsl->isZero()) st.append("MSL"+f_hmsl->text());
  setTitle(st.join(' '));
}
//=============================================================================
void Runway::updateMissionStartPoint()
{
  if(num()!=0)return;
  QGeoCoordinate p(coordinate());
  QGeoCoordinate p2=endPoint();
  group->mission->setStartPoint(p2);
  group->mission->setStartHeading(heading());
  double slen=f_approach->value().toDouble()*2.0-p.distanceTo(endPoint());
  group->mission->setStartLength(slen<0?0:slen);
}
//=============================================================================
void Runway::selectTriggered()
{
  group->mission->vehicle->f_mandala->factByName("rwidx")->setValue(num());
}
//=============================================================================
QGeoCoordinate Runway::endPoint() const
{
  QGeoCoordinate p(coordinate());
  double dN=f_dN->value().toDouble();
  double dE=f_dE->value().toDouble();
  double azimuth=qRadiansToDegrees(atan2(dE,dN));
  double distance=sqrt(pow(dN,2)+pow(dE,2));
  return p.atDistanceAndAzimuth(distance,azimuth);
}
void Runway::setEndPoint(const QGeoCoordinate &v)
{
  QGeoCoordinate p(coordinate());
  double a=qDegreesToRadians(p.azimuthTo(v));
  double d=p.distanceTo(v);
  f_dN->setValue(d*cos(a));
  f_dE->setValue(d*sin(a));
}
QGeoCoordinate Runway::appPoint() const
{
  QGeoCoordinate p(coordinate());
  double dN=f_dN->value().toDouble();
  double dE=f_dE->value().toDouble();
  double azimuth=qRadiansToDegrees(atan2(dE,dN))+180.0;
  double distance=f_approach->value().toDouble();
  return p.atDistanceAndAzimuth(distance,azimuth);
}
void Runway::setAppPoint(const QGeoCoordinate &v)
{
  QGeoCoordinate p(coordinate());
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
