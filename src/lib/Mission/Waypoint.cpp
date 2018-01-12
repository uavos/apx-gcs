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
//=============================================================================
Waypoint::Waypoint(MissionItems *parent)
  : MissionOrderedItem(parent,"W#","",tr("Waypoint"))
{
  f_altitude=new Fact(this,"altitude",tr("Altitude"),tr("Altitude above ground"),FactItem,IntData);

  f_type=new Fact(this,"type",tr("Type"),tr("Maneuver type"),FactItem,EnumData);
  f_type->setEnumStrings(QMetaEnum::fromType<ManeuverType>());

  f_latitude=new Fact(this,"latitude",tr("Latitude"),tr("Global postition latitude"),FactItem,FloatData);
  f_longitude=new Fact(this,"longitude",tr("Longitude"),tr("Global postition longitude"),FactItem,FloatData);


  FactSystem::instance()->jsSync(this);
}
//=============================================================================
