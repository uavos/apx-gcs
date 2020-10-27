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
#include "FirmwareSelect.h"
#include "Firmware.h"

FirmwareSelect::FirmwareSelect(Firmware *firmware,
                               Fact *parent,
                               const QString &name,
                               const QString &title,
                               const QString &descr)
    : Fact(parent, name, title, descr, Group)
    , m_firmware(firmware)
{
    f_node = new Fact(this, "node", tr("Node"), tr("Class of the device"), Enum);
    connect(f_node, &Fact::valueChanged, this, &FirmwareSelect::updateHwEnums);

    f_hw = new Fact(this, "hw", tr("Hardware"), tr("Device hardware"), Enum);

    f_fw = new Fact(this, "fw", tr("Firmware"), tr("Type of firmware"), Enum);
    f_fw->setEnumStrings(QStringList() << "LOADER"
                                       << "FIRMWARE");
    f_start = new Fact(this,
                       "start",
                       tr("Start"),
                       tr("System loader initialization"),
                       Action | Apply,
                       "play");
    f_start->setEnabled(false);
    firmware->f_stop->createAction(this); //link
    connect(firmware->f_stop, &Fact::enabledChanged, f_start, [this]() {
        f_start->setEnabled(!this->m_firmware->f_stop->enabled());
    });

    connect(this, &Fact::triggered, this, &FirmwareSelect::updateNodeEnums);
    updateNodeEnums();
}

void FirmwareSelect::updateNodeEnums()
{
    ApxFw *apxfw = AppGcs::apxfw();
    if (!apxfw->f_current)
        return;
    QStringList st;
    for (int i = 0; i < apxfw->f_current->size(); ++i) {
        st.append(apxfw->f_current->child(i)->name());
    }
    if (apxfw->f_dev) {
        for (int i = 0; i < apxfw->f_dev->size(); ++i) {
            st.append(apxfw->f_dev->child(i)->name());
        }
    }
    st.removeDuplicates();
    st.sort();
    f_node->setEnumStrings(st);
    updateHwEnums();
}

void FirmwareSelect::updateHwEnums()
{
    ApxFw *apxfw = AppGcs::apxfw();
    if (!apxfw->f_current)
        return;
    QStringList st;
    FactBase *f_n = apxfw->f_current->child(f_node->text());
    if (f_n) {
        for (int i = 0; i < f_n->size(); ++i) {
            st.append(f_n->child(i)->name());
        }
    }
    if (apxfw->f_dev) {
        f_n = apxfw->f_dev->child(f_node->text());
        if (f_n) {
            for (int i = 0; i < f_n->size(); ++i) {
                st.append(f_n->child(i)->name());
            }
        }
    }
    st.removeDuplicates();
    st.sort();
    f_hw->setEnumStrings(st);
    f_start->setEnabled(!m_firmware->f_stop->enabled());
}
