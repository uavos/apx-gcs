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
#include "NodesShare.h"
#include "Nodes.h"
#include "NodesStorage.h"

#include <App/AppDirs.h>
#include <App/AppLog.h>
#include <Sharing/NodesXml.h>
#include <Vehicles/Vehicle.h>
//=============================================================================
NodesShare::NodesShare(Nodes *nodes, Fact *parent)
    : Share(parent, tr("Configuration"), "nodes", AppDirs::configs(), QStringList(), Action)
    , nodes(nodes)
{
    connect(this, &Share::imported, nodes->storage, &NodesStorage::loadConfiguration);
}
//=============================================================================
QString NodesShare::defaultExportFileName() const
{
    //QString s=nodes->storage->configInfo.value("title").toString();
    return nodes->vehicle->fileTitle();
}
ShareXmlExport *NodesShare::exportRequest(QString title, QString fileName)
{
    QString hash = nodes->storage->configInfo.value("hash").toString();
    if (hash.isEmpty()) {
        apxMsgW() << tr("Missing config in database");
        return nullptr;
    }
    return new NodesXmlExport(hash, title, fileName);
}
ShareXmlImport *NodesShare::importRequest(QString title, QString fileName)
{
    return new NodesXmlImport(title, fileName);
}
//=============================================================================
//=============================================================================
