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
#include "Loader.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <App/AppGcs.h>
#include <App/AppLog.h>

#include <Protocols/ProtocolServiceFirmware.h>
#include <Protocols/ProtocolVehicles.h>

#include <Nodes/Nodes.h>
#include <Vehicles/Vehicles.h>

#include "LoaderStm.h"
#include "Releases.h"
//=============================================================================
Loader::Loader(Fact *parent, ProtocolServiceFirmware *protocol)
    : QueueItem(parent, "loader")
    , protocol(protocol)
{
    setIcon("autorenew");

    connect(protocol, &ProtocolServiceFirmware::statusChanged, this, [this]() {
        setStatus(this->protocol->status());
    });
    connect(protocol, &ProtocolServiceFirmware::progressChanged, this, [this]() {
        setProgress(this->protocol->progress());
    });

    connect(protocol, &ProtocolServiceFirmware::finished, this, [this](QString, bool success) {
        finish(success);
    });

    connect(this, &Loader::stop, protocol, &ProtocolServiceFirmware::stop);

    setTitle("*");
    setEnabled(false);
}
//=============================================================================
//=============================================================================
void Loader::start(QueueItem *item, Releases *releases)
{
    if (active()) {
        apxMsgW() << tr("Firmware upload in progress");
        return;
    }
    nodeName = item->nodeName;
    nodeDescr = item->nodeDescr;
    sn = item->sn;
    hw = item->hw;
    ver = item->ver;
    type = item->type;
    updateDescr();

    //stage=0;
    //retry=0;
    setActive(true);
    setProgress(0);
    setStatus("");
    setEnabled(true);

    QString relVer = releases->releaseVersion();
    if (ver != relVer) {
        ver = QString("%1->%2").arg(ver).arg(relVer);
    }

    QString s = QString("%1 %2 (%3)").arg(nodeName).arg(hw).arg(ver);
    //if(!nodeDescr.isEmpty())s.append(QString(" (%1)").arg(nodeDescr));
    apxMsg() << QString("%1 (%2): %3")
                    .arg(tr("Firmware upload"))
                    .arg(QMetaEnum::fromType<Firmware::UpgradeType>().valueToKey(type))
                    .arg(s);
    //QTimer::singleShot(500,this,&Loader::next);
    fileData.clear();
    startAddr = 0;
    QString f_fw = nodeName;
    QString f_hw = hw;
    if (!releases->loadFirmware(f_fw, f_hw, type, &fileData, &startAddr)) {
        finish(false);
        return;
    }
    switch (type) {
    default:
        apxMsgW() << "Unsupported upgrade type" << type;
        finish(false);
        break;
    case Firmware::FW:
        protocol->upgradeFirmware(sn, fileData, startAddr);
        break;
    case Firmware::LD:
        protocol->upgradeLoader(sn, fileData, startAddr);
        break;
    case Firmware::STM_LD:
    case Firmware::STM_FW: {
        QStringList opts = nodeDescr.split(',');
        new LoaderStm(this, fileData, startAddr, opts.at(0), QVariant(opts.at(1)).toBool());
    } break;
    }
}
//=============================================================================
void Loader::finish(bool success)
{
    setActive(false);
    setProgress(-1);
    setTitle(status());
    setStatus("");
    setDescr("");
    setEnabled(false);
    emit finished(success);
}
//=============================================================================
//=============================================================================
//=============================================================================
