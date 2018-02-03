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
#include "VehicleSelect.h"
#include "Vehicles.h"
#include "Vehicle.h"
//=============================================================================
VehicleSelect::VehicleSelect(Fact *parent, const QString &name, const QString &title, const QString &descr)
  : Fact(parent,name,title,descr,GroupItem,NoData),
    vehicles(Vehicles::instance())
{
  connect(vehicles,&Vehicles::vehicleRegistered,this,&VehicleSelect::_vehicleRegistered);
  connect(vehicles,&Vehicles::vehicleRemoved,this,&VehicleSelect::_vehicleRemoved);
  connect(vehicles,&Vehicles::vehicleSelected,this,&VehicleSelect::_vehicleSelected);

}
//=============================================================================
void VehicleSelect::_vehicleRegistered(Vehicle *vehicle)
{
  Fact *f=new Fact(this,vehicle->name(),vehicle->title(),vehicle->descr(),FactItem,NoData);
  map.insert(vehicle,f);

  connect(f,&Fact::triggered,this,&VehicleSelect::_factTriggered);

  connect(vehicle,&Vehicle::activeChanged,this,[=](){ f->setActive(vehicle->active()); });
  connect(vehicle,&Fact::statusChanged,this,[=](){ f->setStatus(vehicle->status()); });
}
//=============================================================================
void VehicleSelect::_vehicleRemoved(Vehicle *vehicle)
{
  Fact *f=map.value(vehicle);
  if(!f)return;
  map.remove(vehicle);
  f->remove();
}
//=============================================================================
void VehicleSelect::_vehicleSelected(Vehicle *vehicle)
{
  setStatus(vehicle->title());
}
//=============================================================================
//=============================================================================
void VehicleSelect::_factTriggered()
{
  Vehicle *v=map.key(qobject_cast<Fact*>(sender()));
  if(v) emit vehicleSelected(v);
  emit actionTriggered();
}
//=============================================================================
