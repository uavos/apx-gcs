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
#include "NodesStorage.h"
#include "Nodes.h"

#include <Database/VehiclesReqVehicle.h>

NodesStorage::NodesStorage(Nodes *nodes)
    : QObject(nodes)
    , _nodes(nodes)
{}

void NodesStorage::saveNodesConfig()
{
    qDebug() << _nodes->valid();
    if (!_nodes->valid())
        return;

    if (_nodes->nodes().isEmpty())
        return;

    QList<quint64> nconfIDs;
    for (auto i : _nodes->nodes()) {
        if (!i->valid())
            return;
        auto nconfID = i->storage->configID();
        if (!nconfID)
            return;
        nconfIDs.append(nconfID);
    }
    auto vuid = _nodes->vehicle->uid();
    auto title = _nodes->getConfigTitle();

    auto *req = new DBReqSaveVehicleConfig(vuid, nconfIDs, title);
    req->exec();
}
