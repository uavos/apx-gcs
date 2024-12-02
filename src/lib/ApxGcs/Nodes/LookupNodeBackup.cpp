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
#include "LookupNodeBackup.h"

#include <Database/Database.h>
#include <Database/FleetDB.h>

#include "NodeItem.h"
#include "NodeStorage.h"

LookupNodeBackup::LookupNodeBackup(NodeItem *node, Fact *parent)
    : DatabaseLookup(parent,
                     "backups",
                     tr("Backups"),
                     tr("Restore parameters from backup"),
                     Database::instance()->fleet)
    , _node(node)
{
    connect(this, &DatabaseLookup::itemTriggered, this, &LookupNodeBackup::loadItem);
    QTimer::singleShot(500, this, &DatabaseLookup::defaultLookup);
}

void LookupNodeBackup::loadItem(QVariantMap modelData)
{
    auto hash = modelData.value("hash").toString();
    if (hash.isEmpty())
        return;
    _node->storage->loadNodeConfig(hash);
}

bool LookupNodeBackup::fixItemDataThr(QVariantMap *item)
{
    QString time = QDateTime::fromMSecsSinceEpoch(item->value("time").toLongLong())
                       .toString("yyyy MMM dd hh:mm:ss");
    QString version = item->value("version").toString();
    QString title = item->value("title").toString();
    item->insert("title", time);
    item->insert("value", title);
    item->insert("descr", QString("v%1").arg(version.isEmpty() ? tr("Unknown version") : version));
    return true;
}

void LookupNodeBackup::defaultLookup()
{
    const QString s = QString("%%%1%%").arg(filter());
    query("SELECT * FROM NodeConfigs"
          " LEFT JOIN Nodes ON NodeConfigs.nodeID=Nodes.key "
          " LEFT JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key "
          " WHERE Nodes.sn=? AND (NodeConfigs.title LIKE ?)"
          " ORDER BY NodeConfigs.time DESC"
          " LIMIT 50",
          QVariantList() << _node->uid() << s);
}
