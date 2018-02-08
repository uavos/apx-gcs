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
#include "MissionTools.h"
#include "VehicleMission.h"
#include "Waypoint.h"
#include "Runway.h"
#include "Taxiway.h"
#include "Poi.h"
//=============================================================================
MissionTools::MissionTools(VehicleMission *parent)
  : Fact(parent,"tools",tr("Tools"),tr("Mission edit tools"),GroupItem,NoData),
    mission(parent)
{
  setIcon("wrench");

  f_map=new Fact(this,"map",tr("Map"),tr("Mission map tools"),GroupItem,NoData);
  f_map->setIcon("map");
  Fact *f;

  QString sect(tr("Add object"));
  f=new Fact(f_map,"waypoint",tr("Waypoint"),"",FactItem,NoData);
  f->setIcon("map-marker");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_waypoints,&MissionGroup::add);
  f=new Fact(f_map,"point",tr("Point of interest"),"",FactItem,NoData);
  f->setIcon("map-marker-radius");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_pois,&MissionGroup::add);
  f=new Fact(f_map,"runway",tr("Runway"),"",FactItem,NoData);
  f->setIcon("road");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_runways,&MissionGroup::add);
  f=new Fact(f_map,"taxiway",tr("Taxiway"),"",FactItem,NoData);
  f->setIcon("vector-polyline");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_taxiways,&MissionGroup::add);

  sect=tr("Location");
  f=new Fact(f_map,"home",tr("Set home"),"",FactItem,NoData);
  f->setIcon("home-map-marker");
  f->setSection(sect);
  f=new Fact(f_map,"fly",tr("Fly here"),"",FactItem,NoData);
  f->setIcon("airplane");
  f->setSection(sect);
  f=new Fact(f_map,"look",tr("Look here"),"",FactItem,NoData);
  f->setIcon("eye");
  f->setSection(sect);
  f=new Fact(f_map,"fix",tr("Send position fix"),"",FactItem,NoData);
  f->setIcon("crosshairs-gps");
  f->setSection(sect);

  for (int i=0;i<f_map->size();++i) {
    connect(f_map->childFact(i),&Fact::triggered,f_map,&Fact::actionTriggered);
  }


  f=new Fact(this,"altadjust",tr("Altitude adjust"),tr("Adjust all waypoints altitude"),GroupItem,NoData);
  f->setIcon("altimeter");
  f_altadjust=new Fact(f,"value",tr("Value to add"),"",FactItem,IntData);
  f_altadjust->setUnits("m");
  f_altadjust->setIcon(f->icon());
  connect(f_altadjust,&Fact::valueChanged,this,[=](){
    f_altadjust->setModified(false);
    f_altadjustApply->setEnabled(f_altadjust->value().toInt()!=0);
  });
  f_altadjustApply=new FactAction(f,"apply",tr("Apply"),"","",FactAction::ActionApply|FactAction::ActionCloseOnTrigger);
  f_altadjustApply->setEnabled(false);
  connect(f_altadjustApply,&FactAction::triggered,this,&MissionTools::altadjustTriggered);

  f=new Fact(this,"altset",tr("Altitude set"),tr("Set all waypoints altitude"),GroupItem,NoData);
  f->setIcon("format-align-middle");
  connect(f,&Fact::triggered,this,&MissionTools::updateMaxAltitude);
  f_altset=new Fact(f,"value",tr("Altitude value"),"",FactItem,IntData);
  f_altset->setUnits("m");
  f_altset->setIcon(f->icon());
  f_altset->setMin(0);
  connect(f_altset,&Fact::valueChanged,this,[=](){
    f_altset->setModified(false);
    f_altsetApply->setEnabled(f_altset->value().toInt()!=0);
  });
  f_altsetApply=new FactAction(f,"apply",tr("Apply"),"","",FactAction::ActionApply|FactAction::ActionCloseOnTrigger);
  f_altsetApply->setEnabled(false);
  connect(f_altsetApply,&FactAction::triggered,this,&MissionTools::altsetTriggered);
}
//=============================================================================
void MissionTools::altadjustTriggered()
{
  int v=f_altadjust->value().toInt();
  for(int i=0;i<mission->f_waypoints->size();++i){
    Fact *f=static_cast<Waypoint*>(mission->f_waypoints->child(i))->f_altitude;
    f->setValue(f->value().toInt()+v);
  }
  f_altadjust->setValue(0);
}
//=============================================================================
void MissionTools::altsetTriggered()
{
  int v=f_altset->value().toInt();
  for(int i=0;i<mission->f_waypoints->size();++i){
    Fact *f=static_cast<Waypoint*>(mission->f_waypoints->child(i))->f_altitude;
    f->setValue(v);
  }
}
//=============================================================================
void MissionTools::updateMaxAltitude()
{
  int alt=0;
  for(int i=0;i<mission->f_waypoints->size();++i){
    Waypoint *wp=qobject_cast<Waypoint*>(mission->f_waypoints->childFact(i));
    if(!wp)continue;
    int v=wp->f_altitude->value().toInt();
    if(alt<v)alt=v;
  }
  if(alt>0)f_altset->setValue(alt);
}
//=============================================================================
