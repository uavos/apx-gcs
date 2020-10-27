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
#include "Format.h"
#include "Firmware.h"
#include <App/AppGcs.h>

#include <Nodes/Nodes.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

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

    connect(firmware->nodes_protocol(), &ProtocolNodes::nodeNotify, this, &Format::nodeNotify);

    connect(this, &Fact::triggered, firmware->nodes_protocol(), &ProtocolNodes::requestSearch);

    f_start->setEnabled(false);
}

void Format::nodeNotify(ProtocolNode *protocol)
{
    QStringList n;
    n << protocol->title();
    n << protocol->descr();
    n << protocol->value().toString();
    n.removeAll("");
    QString sTitle = n.join(' ');

    QStringList st = f_dev->enumStrings();

    int idx = snList.indexOf(protocol->sn());
    int v_s = f_dev->value().toInt();
    if (idx < 0) {
        snList << protocol->sn();
        st << sTitle;
    } else {
        st.replace(idx, sTitle);
    }
    f_dev->setEnumStrings(st);
    f_dev->setValue(v_s);
    f_start->setEnabled(true);
}

void Format::startTriggered()
{
    QString sn = snList.value(f_dev->value().toInt());
    QString type;
    if (f_fw->value().toInt() == 0)
        type = "ldr";
    else
        type = "fw";

    if (sn.isEmpty())
        return;

    ProtocolNode *node = Firmware::nodes_protocol()->getNode(sn, true);
    m_firmware->requestFormat(node, type, f_node->text(), f_hw->text());
}
