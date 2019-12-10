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
#include "FirmwareSelect.h"
#include "Firmware.h"
#include "Releases.h"
//=============================================================================
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
//=============================================================================
void FirmwareSelect::updateNodeEnums()
{
    if (!m_firmware->f_releases->f_current)
        return;
    QStringList st;
    for (int i = 0; i < m_firmware->f_releases->f_current->size(); ++i) {
        st.append(m_firmware->f_releases->f_current->child(i)->name());
    }
    if (m_firmware->f_releases->f_dev) {
        for (int i = 0; i < m_firmware->f_releases->f_dev->size(); ++i) {
            st.append(m_firmware->f_releases->f_dev->child(i)->name());
        }
    }
    st.removeDuplicates();
    st.sort();
    f_node->setEnumStrings(st);
    updateHwEnums();
}
//=============================================================================
void FirmwareSelect::updateHwEnums()
{
    if (!m_firmware->f_releases->f_current)
        return;
    QStringList st;
    FactBase *f_n = m_firmware->f_releases->f_current->child(f_node->text());
    if (f_n) {
        for (int i = 0; i < f_n->size(); ++i) {
            st.append(f_n->child(i)->name());
        }
    }
    if (m_firmware->f_releases->f_dev) {
        f_n = m_firmware->f_releases->f_dev->child(f_node->text());
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
//=============================================================================
