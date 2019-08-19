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
#include "NodeToolBlackbox.h"
#include "NodeItem.h"
#include "Nodes.h"
#include <Vehicles/Vehicle.h>
//=============================================================================
NodeToolBlackbox::NodeToolBlackbox(Fact *parent, NodeItem *node, const QString &title)
    : NodeToolsGroup(parent, node, "blackbox", title, "", Group)
{
    setIcon("database");
}
//=============================================================================
Fact *NodeToolBlackbox::addCommand(QString name, QString title, QString descr, uint cmd)
{
    if (name.contains("read", Qt::CaseInsensitive)) {
        Fact *f_read = new Fact(this, name, title, "", Group);
        f_read->setIcon("download");
        f_read->userData = cmd;
        connect(f_read, &Fact::triggered, f_read, &Fact::requestDefaultMenu);

        f_callsign = new Fact(f_read, "callsign", tr("Callsign"), tr("Vehicle identity"), Text);
        if (!node->nodes->vehicle->isLocal()
            && node->nodes->vehicle->vehicleClass() != Vehicle::GCU) {
            f_callsign->setValue(node->nodes->vehicle->callsign());
        }
        f_notes = new Fact(f_read, "notes", tr("Notes"), tr("Telemetry record notes"), Text);

        return nullptr;
    }

    Fact *f = NodeToolsGroup::addCommand(name, title, descr, cmd);
    return f;
}
//=============================================================================
