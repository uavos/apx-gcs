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

    connect(firmware, &Firmware::nodesMapUpdated, this, &Format::nodesMapUpdated);

    f_start->setEnabled(false);
}

void Format::nodesMapUpdated(QMap<QString, QString> nodes)
{
    auto uid_sel = _nodes.key(f_dev->valueText());
    _nodes = nodes;

    f_dev->setEnumStrings(_nodes.values());
    f_dev->setValue(_nodes.value(uid_sel));
    f_start->setEnabled(true);
}

void Format::startTriggered()
{
    auto uid = _nodes.key(f_dev->valueText());
    QString type;
    if (f_fw->value().toInt() == 0)
        type = "ldr";
    else
        type = "fw";

    if (uid.isEmpty())
        return;

    m_firmware->requestUpgrade(uid, f_node->text(), f_hw->text(), type);
}
