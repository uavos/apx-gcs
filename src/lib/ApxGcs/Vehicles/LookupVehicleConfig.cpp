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
#include "LookupVehicleConfig.h"

#include <Database/Database.h>
#include <Database/VehiclesDB.h>

#include <Vehicles/Vehicle.h>

#include "Vehicle.h"

LookupVehicleConfig::LookupVehicleConfig(Vehicle *vehicle, Fact *parent)
    : DatabaseLookup(parent,
                     "load",
                     tr("Load configuration"),
                     tr("Load configuration from database"),
                     Database::instance()->vehicles,
                     Action | IconOnly)
    , _vehicle(vehicle)
{
    connect(this, &DatabaseLookup::itemTriggered, this, &LookupVehicleConfig::loadItem);
}

void LookupVehicleConfig::loadItem(QVariantMap modelData)
{
    QString hash = modelData.value("hash").toString();
    if (hash.isEmpty())
        return;
    _vehicle->storage()->loadVehicleConfig(hash);
}

bool LookupVehicleConfig::fixItemDataThr(QVariantMap *item)
{
    QString time = QDateTime::fromMSecsSinceEpoch(item->value("time").toLongLong())
                       .toString("yyyy MMM dd hh:mm:ss");
    QString callsign = item->value("callsign").toString();
    QString title = item->value("title").toString();
    QString notes = item->value("notes").toString();
    item->insert("title", time);
    item->insert("value", title);
    item->insert("descr", callsign + (notes.isEmpty() ? "" : QString(" - %1").arg(notes)));
    return true;
}

void LookupVehicleConfig::defaultLookup()
{
    const QString s = QString("%%%1%%").arg(filter());
    query("SELECT * FROM VehicleConfigs"
          " LEFT JOIN Vehicles ON VehicleConfigs.vehicleID=Vehicles.key"
          " WHERE callsign LIKE ? OR title LIKE ? OR notes LIKE ? OR class LIKE ?"
          " ORDER BY VehicleConfigs.time DESC",
          QVariantList() << s << s << s << s);
}
