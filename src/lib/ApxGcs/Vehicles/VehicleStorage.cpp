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
#include "VehicleStorage.h"
#include "Vehicle.h"

#include <Nodes/Nodes.h>

#include <Database/VehiclesReqVehicle.h>

VehicleStorage::VehicleStorage(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{}

void VehicleStorage::saveVehicleInfo()
{
    auto m = _vehicle->get_info();
    if (m.isEmpty())
        return;

    auto *req = new DBReqSaveVehicleInfo(m.toVariantMap());
    req->exec();
}

void VehicleStorage::saveVehicleConfig()
{
    auto nodes = _vehicle->f_nodes;

    if (!nodes->valid())
        return;

    if (nodes->nodes().isEmpty())
        return;

    QList<quint64> nconfIDs;
    for (auto i : nodes->nodes()) {
        if (!i->valid())
            return;
        auto nconfID = i->storage->configID();
        if (!nconfID)
            return;
        nconfIDs.append(nconfID);
    }
    auto vuid = _vehicle->uid();
    auto title = nodes->getConfigTitle();

    auto *req = new DBReqSaveVehicleConfig(vuid, nconfIDs, title);
    connect(req,
            &DBReqSaveVehicleConfig::configSaved,
            this,
            &VehicleStorage::configSaved,
            Qt::QueuedConnection);
    req->exec();
}

void VehicleStorage::loadVehicleConfig(QString hash)
{
    auto *req = new DBReqLoadVehicleConfig(hash);
    connect(req,
            &DBReqLoadVehicleConfig::configLoaded,
            this,
            &VehicleStorage::configLoaded,
            Qt::QueuedConnection);
    req->exec();
}
void VehicleStorage::configLoaded(QVariantMap config)
{
    auto title = config.value("title").toString();
    _vehicle->fromVariant(config);
    _vehicle->message(tr("Vehicle configuration loaded").append(": ").append(title));
}

void VehicleStorage::importVehicleConfig(QVariantMap config)
{
    auto *req = new DBReqImportVehicleConfig(config);
    req->exec();
}
