/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "VehicleSelect.h"
#include "Vehicle.h"
#include "Vehicles.h"

VehicleSelect::VehicleSelect(Fact *parent,
                             const QString &name,
                             const QString &title,
                             const QString &descr)
    : Fact(parent, name, title, descr, Group)
    , vehicles(Vehicles::instance())
{
    setEnabled(false);
    connect(vehicles, &Vehicles::vehicleRegistered, this, &VehicleSelect::addVehicle);
    connect(vehicles, &Vehicles::vehicleRemoved, this, &VehicleSelect::_vehicleRemoved);
    connect(vehicles, &Vehicles::vehicleSelected, this, &VehicleSelect::_vehicleSelected);
}

void VehicleSelect::addVehicle(Vehicle *vehicle)
{
    Fact *f = new Fact(this, vehicle->name(), vehicle->title(), vehicle->descr());
    map.insert(vehicle, f);

    connect(f, &Fact::triggered, this, &VehicleSelect::_factTriggered);

    f->bindProperty(vehicle, "visible", true);
    f->bindProperty(vehicle, "active", true);
    f->bindProperty(vehicle, "value", true);
    f->bindProperty(vehicle, "icon", true);

    setEnabled(true);
}

void VehicleSelect::_vehicleRemoved(Vehicle *vehicle)
{
    Fact *f = map.value(vehicle);
    if (!f)
        return;
    map.remove(vehicle);
    f->deleteFact();
    setEnabled(size() > 0);
}

void VehicleSelect::_vehicleSelected(Vehicle *vehicle)
{
    setValue(vehicle->title());
}

void VehicleSelect::_factTriggered()
{
    Vehicle *v = map.key(qobject_cast<Fact *>(sender()));
    if (v)
        emit vehicleSelected(v);
}
