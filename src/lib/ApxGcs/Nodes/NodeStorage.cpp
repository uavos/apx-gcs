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
#include "NodeStorage.h"
#include "NodeItem.h"

#include <Database/VehiclesReqNode.h>

NodeStorage::NodeStorage(NodeItem *node)
    : QObject(node)
    , _node(node)
{}

void NodeStorage::foundID(quint64 key)
{
    _dbKey = key;
}

void NodeStorage::saveNodeInfo()
{
    auto m = _node->get_info().value<QVariantMap>();
    if (m.isEmpty())
        return;

    auto *req = new DBReqVehiclesSaveNodeInfo(m);
    connect(req,
            &DBReqVehiclesSaveNodeInfo::foundID,
            this,
            &NodeStorage::foundID,
            Qt::QueuedConnection);
    req->exec();
}

void NodeStorage::loadNodeInfo()
{
    auto *req = new DBReqVehiclesLoadNodeInfo(_node->uid());
    connect(req,
            &DBReqVehiclesLoadNodeInfo::foundID,
            this,
            &NodeStorage::foundID,
            Qt::QueuedConnection);
    connect(req,
            &DBReqVehiclesLoadNodeInfo::infoLoaded,
            this,
            &NodeStorage::infoLoaded,
            Qt::QueuedConnection);
    req->exec();
}
void NodeStorage::infoLoaded(QVariantMap info)
{
    if (!_node->ident().isEmpty())
        return;
    _node->setTitle(info.value("name").toString());
}
