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
#include "PNodes.h"

#include "PNode.h"
#include "PVehicle.h"

PNodes::PNodes(PVehicle *parent)
    : PTreeBase(parent, "nodes", tr("Nodes"), tr("Vehicle devices"), Group | Count)
{
    connect(this, &PNodes::node_available, this, [this](PNode *node) {
        connect(node, &PNode::upgradingChanged, this, &PNodes::updateUpgrading);
    });
}

void PNodes::updateUpgrading()
{
    bool v = false;
    for (auto i : facts()) {
        auto node = qobject_cast<PNode *>(i);
        if (!node)
            continue;
        if (node->upgrading()) {
            v = true;
            break;
        }
    }
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    emit upgradingChanged();
}
