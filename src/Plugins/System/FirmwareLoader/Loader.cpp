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

#include <Protocols/ProtocolNode.h>
#include <Protocols/ProtocolNodeFile.h>
#include <Protocols/ProtocolNodes.h>

#include <Nodes/Nodes.h>
#include <Vehicles/Vehicles.h>

#include "LoaderStm.h"
#include "Releases.h"

Loader::Loader(Fact *parent)
    : QueueItem(parent, "loader")
{
    setIcon("autorenew");
    setTitle("*");
    setEnabled(false);
}

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
    setValue("");
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
        break;
    case Firmware::FW:
        if (!upgradeFirmware(sn, fileData, startAddr))
            break;
        return;
    case Firmware::LD:
        if (!upgradeLoader(sn, fileData, startAddr))
            break;
        return;
    case Firmware::STM_LD:
    case Firmware::STM_FW: {
        QStringList opts = nodeDescr.split(',');
        new LoaderStm(this, fileData, startAddr, opts.at(0), QVariant(opts.at(1)).toBool());
    }
        return;
    }
    finish(false);
}

void Loader::finish(bool success)
{
    setActive(false);
    setProgress(-1);
    setTitle(value().toString());
    setValue("");
    setDescr("");
    setEnabled(false);
    emit finished(success);
}

ProtocolNodeFile *Loader::lock_file(const QString &node_sn, const QString &fname)
{
    ProtocolNodes *nodes = AppGcs::instance()->protocol->local->nodes;
    ProtocolNode *node = nodes->getNode(node_sn, false);
    if (!node) {
        apxMsgW() << "missing node";
        return nullptr;
    }
    ProtocolNodeFile *file = node->file(fname);
    if (!file) {
        apxMsgW() << "missing node";
        return nullptr;
    }

    if (file_p) {
        disconnect(file_p, nullptr, this, nullptr);
    }
    file_p = file;

    connect(file_p, &ProtocolNodeFile::uploaded, this, [this]() { finish(true); });
    connect(file_p, &ProtocolNodeFile::error, this, [this]() { finish(false); });
    connect(file_p, &ProtocolNodeFile::statusChanged, this, [this]() {
        setValue(file_p->status());
    });
    connect(file_p, &ProtocolNodeFile::progressChanged, this, [this]() {
        setProgress(file_p->progress());
    });
    connect(this, &Loader::stop, file_p, &ProtocolNodeFile::stop);

    return file;
}

bool Loader::upgradeFirmware(QString node_sn, QByteArray data, quint32 offset)
{
    return false;
}

bool Loader::upgradeLoader(QString node_sn, QByteArray data, quint32 offset)
{
    ProtocolNodeFile *file = lock_file(node_sn, "ldr");
    if (!file)
        return false;

    file->upload(data, offset);

    return false;
}
