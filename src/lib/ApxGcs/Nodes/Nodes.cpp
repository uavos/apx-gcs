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
#include "Nodes.h"
#include "NodeField.h"
#include "NodeItem.h"

#include <ApxApp.h>
#include <ApxLog.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
Nodes::Nodes(Vehicle *parent)
    : NodeItemBase(parent, "nodes", "Nodes", Group)
    , vehicle(parent)
    , syncCount(0)
    , syncActive(false)
    , syncUpdate(false)
    , syncUpload(false)
    , m_nodesCount(0)
{
    setIcon("puzzle");
    setDescr(tr("Vehicle components"));

    model()->setFlat(true);

    f_upload = new FactAction(this,
                              "upload",
                              tr("Upload"),
                              tr("Upload modified values"),
                              "upload",
                              FactAction::ActionApply);
    connect(f_upload, &FactAction::triggered, this, &Nodes::upload);

    f_request = new FactAction(this,
                               "request",
                               tr("Request"),
                               tr("Download from vehicle"),
                               "download");
    connect(f_request, &FactAction::triggered, this, &Nodes::request);

    f_reload = new FactAction(this, "reload", tr("Reload"), tr("Clear and download all"), "reload");
    connect(f_reload, &FactAction::triggered, this, &Nodes::reload);

    f_stop = new FactAction(this,
                            "stop",
                            tr("Stop"),
                            tr("Stop data requests"),
                            "",
                            FactAction::ActionStop);
    connect(f_stop, &FactAction::triggered, this, &Nodes::stop);

    f_clear = new FactAction(this,
                             "clear",
                             tr("Clear"),
                             tr("Remove all nodes from list"),
                             "notification-clear-all");
    connect(f_clear, &FactAction::triggered, this, &Nodes::clear);

    f_nstat = new FactAction(this,
                             "nstat",
                             tr("Stats"),
                             tr("Request diagnostics"),
                             "chart-bar-stacked");
    connect(f_nstat, &FactAction::triggered, this, &Nodes::nstat);

    //storage actions
    storage = new NodesStorage(this);

    f_lookup = new LookupConfigs(this, nullptr);
    f_lookup->setParent(this);
    a_lookup = new FactAction(this, f_lookup);

    f_save = new FactAction(this, "save", tr("Save"), tr("Save configuration"), "content-save");
    connect(f_save, &FactAction::triggered, this, &Nodes::save);

    f_share = new NodesShare(this, nullptr);
    f_share->setParent(this);
    a_share = new FactAction(this, f_share);

    foreach (FactAction *a, actions) {
        a->setFlag(FactAction::ActionHideTitle);
        a->setHideDisabled(false);
    }

    connect(this, &Fact::modifiedChanged, this, &Nodes::updateActions);
    connect(this, &Fact::progressChanged, this, &Nodes::updateActions);

    connect(this, &Nodes::nodesCountChanged, this, &Nodes::updateActions);
    updateActions();

    connect(this, &Nodes::nodesCountChanged, this, &Nodes::updateStatus);
    updateStatus();

    syncTimer.setSingleShot(true);
    connect(&syncTimer, &QTimer::timeout, this, &Nodes::sync);

    //protocols
    if (vehicle->protocol) {
        ProtocolService *service = vehicle->protocol->service;
        connect(service, &ProtocolService::nodeFound, this, &Nodes::appendNode);
        connect(service, &ProtocolService::finished, this, &Nodes::syncFinished);

        if (!vehicle->isLocal()) {
            vehicle->protocol->service->requestNodes();
        }

        ProtocolServiceFirmware *fw = Vehicles::instance()->protocol->firmware;
        connect(fw, &ProtocolServiceFirmware::started, this, &Nodes::upgradeStarted);
        connect(fw, &ProtocolServiceFirmware::finished, this, &Nodes::upgradeFinished);
    }

    ApxApp::jsync(this);
}
//=============================================================================
int Nodes::nodesCount() const
{
    return m_nodesCount;
}
//=============================================================================
QVariant Nodes::data(int col, int role) const
{
    if (size() <= 0)
        return Fact::data(col, role);
    return NodesBase::data(col, role);
}
//=============================================================================
//=============================================================================
NodeItem *Nodes::appendNode(const QString &sn, ProtocolServiceNode *protocol)
{
    NodeItem *node = this->node(sn);
    if (node) {
        if (protocol)
            node->setProtocol(protocol);
        return node;
    }
    node = new NodeItem(this, sn, protocol);
    snMap.insert(sn, node);
    m_nodesCount++;
    emit nodesCountChanged();
    return node;
}
void Nodes::removeNode(const QString &sn)
{
    if (vehicle->protocol)
        vehicle->protocol->service->removeNode(sn);
    NodeItem *node = snMap.value(sn);
    if (!node)
        return;
    node->remove();
    snMap.remove(sn);
    m_nodesCount--;
    emit nodesCountChanged();
}
//=============================================================================
void Nodes::updateStatus()
{
    setStatus(nodesCount() > 0 ? QString::number(nodesCount()) : "");
}
void Nodes::updateActions()
{
    bool enb = vehicle->protocol;
    bool busy = progress() >= 0;
    bool upgrading = false; //model->isUpgrading();
    bool bModAll = modified();
    bool bEmpty = nodesCount() <= 0;
    f_request->setEnabled(enb);
    f_upload->setEnabled(enb && bModAll && (!(busy)));
    f_stop->setEnabled(enb && (busy || upgrading));
    f_reload->setEnabled(enb && (!(upgrading || bEmpty)));
    f_clear->setEnabled((!bEmpty) && (!(upgrading || busy)));
    f_nstat->setEnabled(enb && (!bEmpty) && (!(upgrading || busy)));
}
//=============================================================================
void Nodes::syncFinished()
{
    //qDebug()<<vehicle->callsign();
    /*if (vehicle->isLocal()) {
        if (!(dataValid() && dictValid()))
            return;
        vehicle->protocol->service->setActive(false);
        return;
    }*/
    while (1) {
        int cnt = nodesCount();
        if (cnt > 0) {
            if (!(dataValid() && dictValid()))
                break;
            vehicle->protocol->service->setActive(false);
            //exclude reconf nodes
            foreach (NodeItem *node, nodes()) {
                if (node->reconf())
                    cnt--;
            }
            bool bCntChanged = syncCount != cnt;
            syncCount = cnt;
            if (bCntChanged) {
                qDebug() << "sync" << vehicle->callsign()
                         << QString("%1 sec").arg(syncTime.elapsed() / 1000.0, 0, 'f', 1);
                break;
            }
            //all good
            if (!syncUpdate) {
                syncUpdate = true;
                syncUpload = false;
                syncTimestamp = QDateTime::currentDateTimeUtc();
                qDebug() << "save vehicle config on sync done";
                storage->saveConfiguration();
                skipCache.clear();
                //apxMsg() << QString("[%1]").arg(vehicle->callsign())
                //         << tr("Configuration syncronized");
                //qDebug()<<"synced"<<vehicle->callsign()<<QString("%1 sec").arg(syncTime.elapsed()/1000.0,0,'f',1);
                syncLater(5000, false);
            } else if (syncUpload) {
                syncUpload = false;
                syncTimestamp = QDateTime::currentDateTimeUtc();
                qDebug() << "save vehicle config on upload";
                storage->saveConfiguration();
            }
        }
        return;
    }
    syncUpdate = false;
    if (vehicle->isLocal())
        return;
    //schedule re-sync
    syncLater(syncActive ? 100 : 1000, syncActive);
}
void Nodes::sync()
{
    if (!vehicle->protocol)
        return;
    if (vehicle->isTemporary())
        return;
    syncTime.start();
    if (syncActive) {
        vehicle->protocol->service->setActive(true);
    }
    vehicle->protocol->service->requestNodes();
}
void Nodes::uploadedSync()
{
    if (!vehicle->protocol)
        return;
    if (vehicle->isReplay())
        return;
    if (!modified()) {
        syncUpdate = false;
        syncUpload = true;
        syncLater(0);
    }
}
void Nodes::nconfSavedSync()
{
    if (!(dictValid() && dataValid()))
        return;
    //save vehicle config when all nodes done
    if ((vehicle->protocol && vehicle->protocol->service->active()) || syncUpload
        || nodesCount() <= 0)
        return;
    foreach (NodeItem *node, snMap.values()) {
        if (node->nconfID == 0)
            return;
    }
    //all nodes saved configs and not requesting or uploading
    //happens when user save offline nodes
    /*if ((!vehicle->protocol) || vehicle->isReplay()) {
        if (!modified())
            return;
    }*/
    qDebug() << "save vehicle config as all nodes in sync";
    storage->saveConfiguration(true);
}
//=============================================================================
void Nodes::syncLater(int timeout, bool enforce)
{
    syncActive = enforce;
    if (syncTimer.isActive() && timeout > syncTimer.interval())
        timeout = syncTimer.interval();
    syncTimer.start(timeout);
    //qDebug()<<timeout<<vehicle->callsign();
}
//=============================================================================
//=============================================================================
void Nodes::upgradeStarted(QString sn)
{
    NodeItem *node = this->node(sn);
    if (!node)
        return;
    node->setUpgrading(true);
    node->clear();
    ProtocolServiceFirmware *fw = Vehicles::instance()->protocol->firmware;
    connect(fw, &ProtocolServiceFirmware::progressChanged, node, [node, fw]() {
        node->setProgress(fw->progress());
    });
    connect(fw, &ProtocolServiceFirmware::statusChanged, node, [node, fw]() {
        node->setDescr(fw->status());
    });
    connect(f_stop, &FactAction::triggered, fw, &ProtocolServiceFirmware::stop);
}
void Nodes::upgradeFinished(QString sn, bool success)
{
    Q_UNUSED(success)
    NodeItem *node = this->node(sn);
    if (!node)
        return;
    ProtocolServiceFirmware *fw = Vehicles::instance()->protocol->firmware;
    disconnect(fw, nullptr, node, nullptr);
    disconnect(f_stop, nullptr, fw, nullptr);
    node->setUpgrading(false);
    node->updateDescr();
    syncLater(5000);
}
//=============================================================================
//=============================================================================
void Nodes::request()
{
    if (!vehicle->protocol)
        return;
    syncActive = true;
    sync();
}
void Nodes::stop()
{
    if (!vehicle->protocol)
        return;
    vehicle->protocol->service->setActive(false);
    vehicle->protocol->service->stop();
    syncActive = false;
}
//=============================================================================
void Nodes::clear()
{
    if (upgrading())
        return;
    snMap.clear();
    nGroups.clear();
    removeAll();
    if (vehicle->protocol)
        vehicle->protocol->service->clearNodes();
    syncCount = 0;
    m_nodesCount = 0;
    emit nodesCountChanged();
    setModified(false);
    //updateProgress();
    //ApxApp::jsync(this);
}
//=============================================================================
void Nodes::reload()
{
    clear();
    request();
}
//=============================================================================
void Nodes::upload()
{
    if (!(dictValid() && dataValid()))
        return;
    if (!modified())
        return;
    foreach (NodeItem *node, snMap.values()) {
        node->upload();
    }
}
//=============================================================================
void Nodes::save()
{
    if (!(dictValid() && dataValid()))
        return;
    if (!modified())
        return;
    foreach (NodeItem *node, snMap.values()) {
        if (!node->modified())
            continue;
        storage->saveNodeConfig(node);
    }
}
//=============================================================================
void Nodes::nstat()
{
    foreach (NodeItem *node, snMap.values()) {
        node->requestNstat();
    }
}
//=============================================================================
void Nodes::rebootAll()
{
    if (!vehicle->protocol)
        return;
    apxMsg() << tr("Vehicle system reset");
    vehicle->protocol->service->rebootAll();
}
//=============================================================================
void Nodes::clearCache()
{
    skipCache.clear();
    foreach (QString sn, snMap.keys()) {
        skipCache.append(sn);
    }
    apxMsg() << tr("Cache invalidated");
}
//=============================================================================
//=============================================================================
void Nodes::loadConfValue(const QString &sn, QString s)
{
    NodeItem *node = snMap.value(sn);
    if (!node) {
        qWarning() << "missing node" << sn;
        return;
    }
    int del = s.indexOf('=');
    if (del < 0)
        return;
    QString spath = s.left(del).trimmed();
    QString sv = s.mid(del + 1);
    //qDebug()<<spath<<sv;
    if (spath.startsWith(node->title()))
        spath.remove(0, node->title().size() + 1);
    if (spath.isEmpty())
        return;
    node->loadConfigValue(spath, sv);
}
//=============================================================================
