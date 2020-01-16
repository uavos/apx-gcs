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
#ifndef Vehicles_H
#define Vehicles_H
//=============================================================================
#include "Vehicle.h"
#include "VehicleSelect.h"
#include <Fact/Fact.h>
#include <Protocols/ProtocolVehicles.h>
class NodeItem;
//=============================================================================
class Vehicles : public Fact
{
    Q_OBJECT

    Q_PROPERTY(Vehicle *current READ current NOTIFY currentChanged)

public:
    explicit Vehicles(Fact *parent, ProtocolVehicles *protocol);

    static Vehicles *instance() { return _instance; }

    Fact *f_list;

    Vehicle *f_local;
    Vehicle *f_replay;

    VehicleSelect *f_select;

    ProtocolVehicles *protocol;

    Vehicle *createVehicle(ProtocolVehicle *protocol);

private:
    static Vehicles *_instance;

    QList<QMetaObject::Connection> currentVehicleConnections;

public slots:
    void selectVehicle(Vehicle *v);
    void selectPrev();
    void selectNext();

signals:
    void vehicleRegistered(Vehicle *vehicle);
    void vehicleRemoved(Vehicle *vehicle);
    void vehicleSelected(Vehicle *vehicle);

    //data connection
private slots:
    void vehicleIdentified(ProtocolVehicle *protocol);
    void identAssigned(ProtocolVehicle *v, const ProtocolVehicles::IdentData &ident);

public slots:

    //current vehicle signals wrappers
signals:
    void currentDownstreamDataReceived();
    void currentSerialDataReceived(uint portNo, QByteArray ba);

    //forward signals for plugins
    void nodeUpgradeFW(NodeItem *node);
    void nodeUpgradeLD(NodeItem *node);
    void nodeUpgradeMHX(NodeItem *node);

    void nodeNotify(NodeItem *node); //node is available and info updated

    //---------------------------------------
    // PROPERTIES
public:
    Vehicle *current(void) const;

protected:
    Vehicle *m_current;

signals:
    void currentChanged();
};
//=============================================================================
#endif
