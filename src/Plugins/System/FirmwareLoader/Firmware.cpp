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
#include "Firmware.h"
#include "FirmwareTools.h"
#include "Loader.h"
#include "QueueItem.h"
#include "Releases.h"
#include <App/AppLog.h>

#include <App/App.h>
#include <Nodes/NodeItem.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
APX_LOGGING_CATEGORY(FirmwareLog, "core.Firmware")
//=============================================================================
Firmware *Firmware::_instance = nullptr;
Firmware::Firmware(Fact *parent, ProtocolServiceFirmware *protocol)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Firmware"),
           tr("Firmware updates"),
           Group | FlatModel,
           "update")
    , protocol(protocol)
{
    _instance = this;

    f_loader = new Loader(this, protocol);
    connect(f_loader, &Loader::finished, this, &Firmware::loaderFinished);

    f_releases = new Releases(this);

    f_queue = new Fact(this, "queue", tr("Queue"), tr("Nodes to upgrade"), Section | Const);
    f_queue->setIcon("playlist-play");

    f_available = new Fact(this,
                           "available",
                           tr("Available"),
                           tr("Upgradable nodes"),
                           Section | Const);
    f_available->setIcon("star-circle");

    f_upgrade = new Fact(this,
                         "upgrade",
                         tr("Upgrade"),
                         tr("Auto upgrade all nodes"),
                         Action,
                         "auto-upload");

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop upgrading"), Action | Stop);
    connect(f_stop, &Fact::triggered, this, &Firmware::stop);

    //tools actions
    f_tools = new FirmwareTools(this);

    connect(f_queue, &Fact::sizeChanged, this, &Firmware::updateStatus);
    connect(f_available, &Fact::sizeChanged, this, &Firmware::updateStatus);
    connect(this, &Fact::activeChanged, this, &Firmware::updateStatus);

    connect(this, &Fact::activeChanged, this, &Firmware::updateProgress);
    connect(f_loader, &Loader::progressChanged, this, &Firmware::updateProgress);
    connect(f_queue, &Fact::sizeChanged, this, &Firmware::updateProgress);

    updateStatus();

    connect(Vehicles::instance(), &Vehicles::nodeNotify, this, &Firmware::nodeNotify);
    connect(Vehicles::instance(), &Vehicles::nodeUpgradeFW, this, [this](NodeItem *node) {
        requestUpgrade(node->title(),
                       node->descr(),
                       node->sn(),
                       node->hardware(),
                       node->version(),
                       Firmware::FW);
    });
    connect(Vehicles::instance(), &Vehicles::nodeUpgradeLD, this, [this](NodeItem *node) {
        requestUpgrade(node->title(),
                       node->descr(),
                       node->sn(),
                       node->hardware(),
                       node->version(),
                       Firmware::LD);
    });
    connect(Vehicles::instance(), &Vehicles::nodeUpgradeMHX, this, [this](NodeItem *node) {
        requestUpgrade(node->title(),
                       node->descr(),
                       node->sn(),
                       node->hardware(),
                       node->version(),
                       Firmware::MHX);
    });

    //App::jsync(this);
}
//=============================================================================
void Firmware::updateStatus()
{
    bool act = active();
    //f_loader->setVisible(act);
    //actions
    f_stop->setEnabled(act);
    f_upgrade->setEnabled(f_available->size() > 0);
    if (act) {
        setStatus(QString("%1/%2").arg(queueCnt - f_queue->size()).arg(queueCnt));
    } else {
        queueCnt = 0;
        setStatus("");
    }
}
void Firmware::updateProgress()
{
    if (!active()) {
        setProgress(-1);
        return;
    }
    setProgress((queueCnt - f_queue->size() - 1) * 100 / queueCnt + f_loader->progress() / queueCnt);
}
//=============================================================================
void Firmware::requestUpgrade(const QString &nodeName,
                              const QString &nodeDescr,
                              const QString &sn,
                              const QString &hw,
                              const QString &ver,
                              UpgradeType type)
{
    QueueItem *f = queued(f_available, sn, Any);
    if (f)
        f->remove();
    f = queued(f_queue, sn, type);
    if (f) {
    } else if ((!(f_loader->active() && f_loader->sn == sn && f_loader->type == type))) {
        new QueueItem(f_queue, nodeName, nodeDescr, sn, hw, ver, type);
        queueCnt++;
    }
    if (!active())
        next();
    else
        updateStatus();
    trigger();
}
void Firmware::requestInitialization(const QString &nodeName,
                                     const QString &hw,
                                     const QString &portName,
                                     UpgradeType type)
{
    requestUpgrade(nodeName, portName, "any", hw, f_releases->releaseVersion(), type);
}
//=============================================================================
void Firmware::nodeNotify(NodeItem *node)
{
    if (!node->fwSupport())
        return;
    if (!node->infoValid())
        return;

    if (queued(f_queue, node->sn(), Any))
        return;
    if (f_loader->active() && f_loader->sn == node->sn())
        return;
    if (!queued(f_available, node->sn(), Any)) {
        new QueueItem(f_available,
                      node->title(),
                      node->status(),
                      node->sn(),
                      node->hardware(),
                      node->version(),
                      Any);
        return;
    }
    if (node->status().isEmpty())
        return;
    for (int i = 0; i < f_available->size(); ++i) {
        QueueItem *f = static_cast<QueueItem *>(f_available->child(i));
        if (f->sn != node->sn())
            continue;
        f->nodeDescr = node->status();
        f->updateDescr();
    }
}
//=============================================================================
QueueItem *Firmware::queued(Fact *list, const QString &sn, UpgradeType type)
{
    for (int i = 0; i < list->size(); ++i) {
        QueueItem *f = static_cast<QueueItem *>(list->child(i));
        if (f->sn != sn)
            continue;
        if (type != Any && f->type != type)
            continue;
        return f;
    }
    return nullptr;
}
//=============================================================================
//=============================================================================
void Firmware::next()
{
    if (f_queue->size() <= 0) {
        setActive(false);
        return;
    }
    setActive(true);
    //find next node in queue
    QueueItem *item = nullptr;
    //loaders first
    for (int i = 0; item == nullptr && i < f_queue->size(); ++i) {
        QueueItem *f = static_cast<QueueItem *>(f_queue->child(i));
        if (f->type == LD)
            item = f;
    }
    //sorted by names
    QStringList names;
    names << "servo"
          << "gps"
          << "cas"
          << "nav"
          << "ifc"
          << "";
    foreach (QString s, names) {
        for (int i = 0; item == nullptr && i < f_queue->size(); ++i) {
            QueueItem *f = static_cast<QueueItem *>(f_queue->child(i));
            if (s.isEmpty() || f->nodeName == s) {
                item = f;
            }
        }
        if (item)
            break;
    }
    if (!item)
        return;
    emit upgradeStarted(item->sn, item->type);
    f_loader->start(item, f_releases);
    item->remove();
}
//=============================================================================
void Firmware::loaderFinished(bool success)
{
    emit upgradeFinished(f_loader->sn, f_loader->type);
    if (!success)
        f_queue->removeAll();
    QTimer::singleShot(1, this, &Firmware::next);
}
//=============================================================================
void Firmware::stop()
{
    f_queue->removeAll();
    f_loader->stop();
}
//=============================================================================
//=============================================================================
