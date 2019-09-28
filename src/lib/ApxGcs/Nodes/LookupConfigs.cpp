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
#include "LookupConfigs.h"
#include <Database/Database.h>
#include <Database/NodesDB.h>

#include <Nodes/Nodes.h>
#include <Vehicles/Vehicle.h>
//=============================================================================
LookupConfigs::LookupConfigs(Nodes *nodes, Fact *parent)
    : DatabaseLookup(parent,
                     "load",
                     tr("Load configuration"),
                     tr("Database lookup"),
                     Database::instance()->nodes,
                     Action)
    , nodes(nodes)
{
    connect(this, &DatabaseLookup::itemTriggered, this, &LookupConfigs::loadItem);
}
//=============================================================================
void LookupConfigs::loadItem(QVariantMap modelData)
{
    QString hash = modelData.value("hash").toString();
    if (hash.isEmpty())
        return;
    nodes->storage->loadConfiguration(hash);
}
//=============================================================================
bool LookupConfigs::fixItemDataThr(QVariantMap *item)
{
    QString time = QDateTime::fromMSecsSinceEpoch(item->value("time").toLongLong())
                       .toString("yyyy MMM dd hh:mm:ss");
    QString callsign = item->value("callsign").toString();
    QString title = item->value("title").toString();
    QString notes = item->value("notes").toString();
    item->insert("title", time);
    item->insert("status", title);
    item->insert("descr", callsign + (notes.isEmpty() ? "" : QString(" - %1").arg(notes)));
    return true;
}
//=============================================================================
//=============================================================================
//=============================================================================
void LookupConfigs::defaultLookup()
{
    const QString s = QString("%%%1%%").arg(filter());
    query("SELECT * FROM VehicleConfigs"
          " LEFT JOIN Vehicles ON VehicleConfigs.vehicleID=Vehicles.key"
          " WHERE callsign LIKE ? OR title LIKE ? OR notes LIKE ? OR class LIKE ?"
          " ORDER BY VehicleConfigs.time DESC",
          QVariantList() << s << s << s << s);
}
//=============================================================================
//=============================================================================
