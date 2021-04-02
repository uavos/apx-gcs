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

void NodeStorage::saveNodeInfo()
{
    auto info = _node->get_info();
    if (info.isEmpty())
        return;

    auto *req = new DBReqSaveNodeInfo(info);
    req->exec();
}

void NodeStorage::saveNodeDict()
{
    auto dict = _node->get_dict();
    if (dict.isEmpty()) {
        qWarning() << "no dict data";
        return;
    }

    auto *req = new DBReqSaveNodeDict(_node->uid(), dict);
    req->exec();
}

void NodeStorage::saveNodeConfig()
{
    auto hash = _node->get_dict().value("hash").toString();
    if (hash.isEmpty()) {
        qWarning() << "no dict hash";
        return;
    }
    auto *req = new DBReqSaveNodeConfig(_node->uid(), hash, _node->get_values());
    req->exec();
}

void NodeStorage::loadNodeConfig(QString hash)
{
    auto *req = new DBReqLoadNodeConfig(_node->uid(), hash);
    connect(req,
            &DBReqLoadNodeConfig::configLoaded,
            _node,
            &NodeItem::importValues,
            Qt::QueuedConnection);
    req->exec();
}
