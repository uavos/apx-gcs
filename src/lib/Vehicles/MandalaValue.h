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
#ifndef MandalaValue_H
#define MandalaValue_H
//=============================================================================
#include <QtCore>
#include <QVariant>
#include "Mandala.h"
#include "FactValue.h"
#include "Vehicles.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
#include "VehicleMandalaFact.h"
//=============================================================================
template<int id,typename T>
class VehicleMandalaValue : public FactValue<T>
{
public:
  VehicleMandalaValue(Vehicle *vehicle)
    : FactValue<T>(NULL)
  {
    updateFact(vehicle);
  }
protected:
  void updateFact(Vehicle *vehicle)
  {
    FactValue<T>::f=vehicle->f_mandala->factById(id);
    if(!FactValue<T>::f){
      qWarning("MandalaValue ID(%u) not found for vehicle %s",id,vehicle->title().toUtf8().data());
    }
  }
};
//=============================================================================
template<int id,typename T>
class MandalaValue : public QObject, public VehicleMandalaValue<id,T>
{
public:
  MandalaValue(QObject *parent =0)
   : QObject(parent), VehicleMandalaValue<id,T>(Vehicles::instance()->current())
  {
    connect(Vehicles::instance(),&Vehicles::vehicleSelected,[=](Vehicle *v){ VehicleMandalaValue<id,T>::updateFact(v); });
  }
};
//=============================================================================
#endif

