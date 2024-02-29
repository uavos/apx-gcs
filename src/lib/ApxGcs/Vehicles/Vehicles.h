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
#pragma once

#include <Protocols/Protocols.h>

#include "Vehicle.h"
#include "VehicleSelect.h"

#include <App/AppEngine.h>

class Vehicle;

class Vehicles : public Fact
{
    Q_OBJECT

    Q_PROPERTY(Vehicle *current READ current NOTIFY currentChanged)
    Q_PROPERTY(Vehicle *gcs READ gcs NOTIFY gcsChanged)

public:
    explicit Vehicles(Fact *parent, Protocols *protocols);

    static Vehicles *instance() { return _instance; }

    static constexpr const int list_padding = 2;
    Vehicle *f_replay;

    VehicleSelect *f_select;

public:
    Vehicle *current(void) const { return m_current; }
    Vehicle *gcs(void) const { return m_gcs; }

protected:
    QPointer<Vehicle> m_current;
    QPointer<Vehicle> m_gcs;

private:
    static Vehicles *_instance;

    void _jsSyncMandalaAccess(Fact *fact, QJSValue parent);

    void _update_gcs(Vehicle *vehicle);

public slots:
    void selectVehicle(Vehicle *v);
    void selectPrev();
    void selectNext();

    void deleteVehicle(Vehicle *v);
    void clearAll();

    //data connection
private slots:
    void vehicle_available(PVehicle *protocol);

signals:
    void vehicleRegistered(Vehicle *vehicle);
    void vehicleRemoved(Vehicle *vehicle);
    void vehicleSelected(Vehicle *vehicle);

signals:
    void currentChanged();
    void gcsChanged();
};
