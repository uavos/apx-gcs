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
#include "Initialize.h"
#include "Firmware.h"
#include "Releases.h"
#include <App/App.h>
#include <App/AppGcs.h>
#include <App/AppLog.h>
#include <QSerialPortInfo>
//=============================================================================
Initialize::Initialize(Firmware *firmware, Fact *parent)
    : Fact(parent, "initialize", tr("Initialize"), tr("Flash device via system loader"), Group)
    , firmware(firmware)
{
    setIcon("new-box");

    f_node = new Fact(this, "node", tr("Node"), tr("Class of the device"), Enum);
    connect(f_node, &Fact::valueChanged, this, &Initialize::updateHwEnums);

    f_hw = new Fact(this, "hw", tr("Hardware"), tr("Device hardware"), Enum);

    f_fw = new Fact(this, "fw", tr("Firmware"), tr("Type of firmware"), Enum);
    f_fw->setEnumStrings(QStringList() << "LOADER"
                                       << "FIRMWARE");

    f_port = new Fact(this, "port", tr("Port"), tr("Serial device"), Enum);

    f_continuous = new Fact(this, "continuous", tr("Continuous"), tr("Stop on user action"), Bool);
    f_continuous->setValue(true);

    f_start = new Fact(this,
                       "start",
                       tr("Start"),
                       tr("System loader initialization"),
                       Action | Apply,
                       "play");
    f_start->setEnabled(false);
    firmware->f_stop->createAction(this); //link
    connect(firmware->f_stop, &Fact::enabledChanged, f_start, [this]() {
        f_start->setEnabled(!this->firmware->f_stop->enabled());
    });
    connect(f_start, &Fact::triggered, this, &Initialize::startTriggered);

    connect(this, &Fact::triggered, this, &Initialize::updateNodeEnums);
    updateNodeEnums();
}
//=============================================================================
void Initialize::updateNodeEnums()
{
    updatePortEnums();
    if (!firmware->f_releases->f_current)
        return;
    QStringList st;
    for (int i = 0; i < firmware->f_releases->f_current->size(); ++i) {
        st.append(firmware->f_releases->f_current->child(i)->name());
    }
    if (firmware->f_releases->f_dev) {
        for (int i = 0; i < firmware->f_releases->f_dev->size(); ++i) {
            st.append(firmware->f_releases->f_dev->child(i)->name());
        }
    }
    st.removeDuplicates();
    f_node->setEnumStrings(st);
    updateHwEnums();
}
//=============================================================================
void Initialize::updateHwEnums()
{
    if (!firmware->f_releases->f_current)
        return;
    QStringList st;
    FactBase *f_n = firmware->f_releases->f_current->child(f_node->text());
    if (!f_n) {
        if (firmware->f_releases->f_dev)
            f_n = firmware->f_releases->f_dev->child(f_node->text());
        if (!f_n)
            return;
    }
    for (int i = 0; i < f_n->size(); ++i) {
        st.append(f_n->child(i)->name());
    }
    f_hw->setEnumStrings(st);
    f_start->setEnabled(!firmware->f_stop->enabled());
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
    Firmware::UpgradeType type;
    if (f_fw->value().toInt() == 0)
        type = Firmware::STM_LD;
    else
        type = Firmware::STM_FW;
    QStringList opts;
    opts << f_port->text();
    opts << f_continuous->text();
    firmware->requestInitialization(f_node->text(), f_hw->text(), opts.join(','), type);
}
//=============================================================================
//=============================================================================
