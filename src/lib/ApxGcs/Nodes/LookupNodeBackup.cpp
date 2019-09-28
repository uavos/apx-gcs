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
#include "LookupNodeBackup.h"
#include "NodeItem.h"
#include "Nodes.h"
#include <Database/Database.h>
#include <Database/NodesDB.h>
//=============================================================================
LookupNodeBackup::LookupNodeBackup(NodeItem *node, Fact *parent)
    : DatabaseLookup(parent,
                     "backups",
                     tr("Backups"),
                     tr("Restore parameters from backup"),
                     Database::instance()->nodes)
    , node(node)
{
    connect(this, &DatabaseLookup::itemTriggered, this, &LookupNodeBackup::loadItem);
    QTimer::singleShot(500, this, &DatabaseLookup::defaultLookup);
}
//=============================================================================
void LookupNodeBackup::loadItem(QVariantMap modelData)
{
    quint64 nconfID = modelData.value("key", 0).toULongLong();
    if (!nconfID)
        return;
    node->nodes->storage->loadNodeConfig(node, nconfID);
}
//=============================================================================
bool LookupNodeBackup::fixItemDataThr(QVariantMap *item)
{
    QString time = QDateTime::fromMSecsSinceEpoch(item->value("time").toLongLong())
                       .toString("yyyy MMM dd hh:mm:ss");
    QString version = item->value("version").toString();
    QString title = item->value("title").toString();
    item->insert("title", time);
    item->insert("status", title);
    item->insert("descr",
                 version == node->version()
                     ? tr("Current release")
                     : QString("v%1").arg(version.isEmpty() ? tr("Unknown version") : version));
    return true;
}
//=============================================================================
//=============================================================================
//=============================================================================
void LookupNodeBackup::defaultLookup()
{
    const QString s = QString("%%%1%%").arg(filter());
    query("SELECT * FROM NodeConfigs"
          " LEFT JOIN Nodes ON NodeConfigs.nodeID=Nodes.key "
          " LEFT JOIN NodeDicts ON NodeConfigs.dictID=NodeDicts.key "
          " WHERE Nodes.sn=? AND (NodeConfigs.title LIKE ?)"
          " ORDER BY NodeConfigs.time DESC"
          " LIMIT 50",
          QVariantList() << node->sn() << s);
}
//=============================================================================
//=============================================================================
