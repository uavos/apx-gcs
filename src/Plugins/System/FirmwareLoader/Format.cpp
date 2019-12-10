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
#include "Format.h"
#include "Firmware.h"
#include <App/AppGcs.h>

#include <Nodes/Nodes.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
Format::Format(Firmware *firmware, Fact *parent)
    : FirmwareSelect(firmware,
                     parent,
                     "format",
                     tr("Format"),
                     tr("Flash device with specific firmare"))
{
    setIcon("format-rotate-90");

    f_dev = new Fact(this, "dev", tr("Device"), tr("Node device"), Enum);

    connect(f_start, &Fact::triggered, this, &Format::startTriggered);

    connect(Vehicles::instance(), &Vehicles::nodeNotify, this, &Format::nodeNotify);

    Nodes *nodes = Vehicles::instance()->f_local->f_nodes;
    connect(this, &Fact::triggered, nodes, &Nodes::request);

    f_start->setEnabled(false);
}
//=============================================================================
void Format::nodeNotify(NodeItem *node)
{
    if (!node->fwSupport())
        return;

    QStringList n;
    n << node->title();
    n << node->descr();
    n << node->status();
    n.removeAll("");
    QString sTitle = n.join(' ');

    QStringList st = f_dev->enumStrings();

    int idx = snList.indexOf(node->sn());
    int v_s = f_dev->value().toInt();
    if (idx < 0) {
        snList << node->sn();
        st << sTitle;
    } else {
        st.replace(idx, sTitle);
    }
    f_dev->setEnumStrings(st);
    f_dev->setValue(v_s);
    f_start->setEnabled(true);
}
//=============================================================================
//=============================================================================
void Format::startTriggered()
{
    Firmware::UpgradeType type;
    if (f_fw->value().toInt() == 0)
        type = Firmware::LD;
    else
        type = Firmware::FW;

    QString nodeName = f_node->text();
    QString nodeDescr = f_dev->text();
    QString sn = snList.value(f_dev->value().toInt());
    QString hw = f_hw->text();

    if (sn.isEmpty())
        return;
    //apxMsgW() << tr("Using custom firmware file").append(':') << s;

    m_firmware->requestUpgrade(nodeName, nodeDescr, sn, hw, "any", type);
}
//=============================================================================
//=============================================================================
