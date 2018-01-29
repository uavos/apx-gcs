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
  setIconSource("wrench");

  f_map=new Fact(this,"map",tr("Map"),tr("Mission map tools"),GroupItem,NoData);
  f_map->setIconSource("map");
  Fact *f;

  QString sect(tr("Add object"));
  f=new Fact(f_map,"waypoint",tr("Waypoint"),"",FactItem,ActionData);
  f->setIconSource("map-marker");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_waypoints,&MissionGroup::add);
  f=new Fact(f_map,"point",tr("Point of interest"),"",FactItem,ActionData);
  f->setIconSource("map-marker-radius");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_pois,&MissionGroup::add);
  f=new Fact(f_map,"runway",tr("Runway"),"",FactItem,ActionData);
  f->setIconSource("road");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_runways,&MissionGroup::add);
  f=new Fact(f_map,"taxiway",tr("Taxiway"),"",FactItem,ActionData);
  f->setIconSource("vector-polyline");
  f->setSection(sect);
  connect(f,&Fact::triggered,mission->f_taxiways,&MissionGroup::add);

  sect=tr("Location");
  f=new Fact(f_map,"home",tr("Set home"),"",FactItem,ActionData);
  f->setIconSource("home-map-marker");
  f->setSection(sect);
  f=new Fact(f_map,"fly",tr("Fly here"),"",FactItem,ActionData);
  f->setIconSource("airplane");
  f->setSection(sect);
  f=new Fact(f_map,"look",tr("Look here"),"",FactItem,ActionData);
  f->setIconSource("eye");
  f->setSection(sect);
  f=new Fact(f_map,"fix",tr("Send position fix"),"",FactItem,ActionData);
  f->setIconSource("crosshairs-gps");
  f->setSection(sect);


  f=new Fact(this,"altadjust",tr("Altitude adjust"),tr("Adjust all waypoints altitude"),GroupItem,NoData);
  f->setIconSource("altimeter");
  f_altadjust=new Fact(f,"value",tr("Value to add"),"",FactItem,IntData);
  f_altadjust->setUnits("m");
  f_altadjust->setIconSource(f->iconSource());
  connect(f_altadjust,&Fact::valueChanged,this,[=](){
    f_altadjust->setModified(false);
    f_altadjustApply->setEnabled(f_altadjust->value().toInt()!=0);
  });
  f_altadjustApply=new Fact(f,"apply",tr("Apply"),"",FactItem,ActionData);
  f_altadjustApply->setValue(ApplyAction);
  f_altadjustApply->setEnabled(false);
  connect(f_altadjustApply,&Fact::triggered,this,&MissionTools::altadjustTriggered);

  f=new Fact(this,"altset",tr("Altitude set"),tr("Set all waypoints altitude"),GroupItem,NoData);
  f->setIconSource("format-align-middle");
  f_altset=new Fact(f,"value",tr("Altitude value"),"",FactItem,IntData);
  f_altset->setUnits("m");
  f_altset->setIconSource(f->iconSource());
  f_altset->setMin(0);
  connect(f_altset,&Fact::valueChanged,this,[=](){
    f_altset->setModified(false);
    f_altsetApply->setEnabled(f_altset->value().toInt()!=0);
  });
  f_altsetApply=new Fact(f,"apply",tr("Apply"),"",FactItem,ActionData);
  f_altsetApply->setValue(ApplyAction);
  f_altsetApply->setEnabled(false);
  connect(f_altsetApply,&Fact::triggered,this,&MissionTools::altsetTriggered);
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
