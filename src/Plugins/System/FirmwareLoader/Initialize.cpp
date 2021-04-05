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
#include "Initialize.h"
#include "Firmware.h"
#include <App/AppGcs.h>
#include <QSerialPortInfo>
//=============================================================================
Initialize::Initialize(Firmware *firmware, Fact *parent)
    : FirmwareSelect(firmware,
                     parent,
                     "initialize",
                     tr("Initialize"),
                     tr("Flash device via system loader"))
{
    setIcon("new-box");

    f_port = new Fact(this, "port", tr("Port"), tr("Serial device"), Enum);

    f_continuous = new Fact(this, "continuous", tr("Continuous"), tr("Stop on user action"), Bool);
    f_continuous->setValue(true);

    connect(f_start, &Fact::triggered, this, &Initialize::startTriggered);

    connect(this, &Fact::triggered, this, &Initialize::updatePortEnums);
    updatePortEnums();
}
//=============================================================================
void Initialize::updatePortEnums()
{
    QStringList st;
    foreach (QSerialPortInfo spi, QSerialPortInfo::availablePorts()) {
        if (st.contains(spi.portName()))
            continue;
        st.append(spi.portName());
    }
    f_port->setEnumStrings(st);
    //select active port
    QStringList sta = AppGcs::instance()->f_datalink->f_ports->activeSerialPorts();
    if (!sta.isEmpty())
        f_port->setValue(sta.first());
    else {
        QString v, vusb;
        foreach (QString s, st) {
            bool cu = s.contains("cu.", Qt::CaseInsensitive);
            bool usb = s.contains("usb", Qt::CaseInsensitive);
            if (cu && usb) {
                v = s;
                break;
            }
            if (usb)
                vusb = s;
        }
        if (!v.isEmpty())
            f_port->setValue(v);
        else if (!vusb.isEmpty())
            f_port->setValue(vusb);
    }
}
//=============================================================================
//=============================================================================
void Initialize::startTriggered()
{
    QString type;
    if (f_fw->value().toInt() == 0)
        type = "ldr";
    else
        type = "fw";
    m_firmware->requestInitialize(f_node->text(),
                                  f_hw->text(),
                                  type,
                                  f_port->text(),
                                  f_continuous->value().toBool());
}
//=============================================================================
//=============================================================================
