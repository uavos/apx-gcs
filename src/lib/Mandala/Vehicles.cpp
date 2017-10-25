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
#include "Vehicles.h"
#include "Vehicle.h"
//=============================================================================
Vehicles::Vehicles(FactSystem *parent)
  : Fact(parent,"vehicles",tr("Vehicles"),tr("Discovered unmanned vehicles"),SectionItem,NoData)
{
  setFlatModel(true);

  f_select=new Fact(this,"select",tr("Select vehicle"),tr("Change the active vehicle"),GroupItem,NoData);
  f_select->setSection(title());

  f_list=new Fact(this,"list",tr("Vehicles list"),"",SectionItem,ConstData);
  bind(f_list);

  Vehicle *v;
  v=new Vehicle(this,"LOCAL",NULL);
  v->setDescr("UAV Squawk: E2E4");
  v->setStatus("XPDR");

  v=new Vehicle(this,"SGCU",NULL);
  v->setDescr("GCU Squawk: E2E4");
  v->setStatus("XPDR");

  v=new Vehicle(this,"SAKER-1B2",NULL);
  v->setDescr("UAV Squawk: E2E4");
  v->setStatus("ACTIVE");

}
//=============================================================================
void Vehicles::downlinkReceived(const QByteArray &ba)
{
}
//=============================================================================
//=============================================================================
