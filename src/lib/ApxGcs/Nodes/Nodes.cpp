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

#include <App/App.h>
#include <App/AppLog.h>
#include <Vehicles/Vehicles.h>

Nodes::Nodes(Vehicle *parent)
    : NodeItemBase(parent, "nodes", "Nodes", Group | FlatModel | ModifiedGroup)
    , vehicle(parent)
    , m_protocol(vehicle->protocol ? vehicle->protocol->nodes : nullptr)
{
    setIcon("puzzle");
    setDescr(tr("Vehicle components"));

    f_upload = new Fact(this,
                        "upload",
                        tr("Upload"),
                        tr("Upload modified values"),
                        Action | Apply,
                        "upload");
    connect(f_upload, &Fact::triggered, this, &Nodes::upload);

    f_search
        = new Fact(this, "search", tr("Search"), tr("Download from vehicle"), Action, "download");
    connect(f_search, &Fact::triggered, this, &Nodes::search);

    f_reload = new Fact(this, "reload", tr("Reload"), tr("Clear and download all"), Action, "reload");
    connect(f_reload, &Fact::triggered, this, &Nodes::reload);

    f_stop = new Fact(this, "stop", tr("Stop"), tr("Stop data requests"), Action | Stop);
    connect(f_stop, &Fact::triggered, this, &Nodes::stop);

    f_clear = new Fact(this,
                       "clear",
                       tr("Clear"),
                       tr("Remove all nodes from list"),
                       Action,
                       "notification-clear-all");
    connect(f_clear, &Fact::triggered, this, &Nodes::clear);

    f_status
        = new Fact(this, "status", tr("Status"), tr("Request status"), Action, "chart-bar-stacked");
    connect(f_status, &Fact::triggered, this, &Nodes::status);

    //storage actions
    /*storage = new NodesStorage(this);

    f_lookup = new LookupConfigs(this, this);

    f_save = new Fact(this, "save", tr("Save"), tr("Save configuration"), Action, "content-save");
    connect(f_save, &Fact::triggered, this, &Nodes::save);

    f_share = new NodesShare(this, this);*/

    foreach (FactBase *a, actions()) {
        a->setOption(IconOnly);
        a->setOption(ShowDisabled);
    }

    connect(this, &Fact::modifiedChanged, this, &Nodes::updateActions);
    connect(this, &Fact::progressChanged, this, &Nodes::updateActions);

    connect(this, &Nodes::nodesCountChanged, this, &Nodes::updateActions);
    updateActions();

    connect(this, &Nodes::nodesCountChanged, this, &Nodes::updateStatus);
    updateStatus();

    m_syncTimer.setSingleShot(true);
    connect(&m_syncTimer, &QTimer::timeout, this, &Nodes::sync);

    //protocols
    if (m_protocol) {
        connect(m_protocol, &ProtocolNodes::nodeFound, this, &Nodes::addNode);
        connect(m_protocol, &ProtocolNodes::finished, this, &Nodes::protocolFinished);

        connect(this, &Fact::activeChanged, m_protocol, [this]() {
            m_protocol->setActive(active());
        });

        if (!vehicle->isLocal()) {
            m_protocol->requestSearch();
        }

        /*ProtocolServiceFirmware *fw = Vehicles::instance()->protocol->firmware;
        connect(fw, &ProtocolServiceFirmware::started, this, &Nodes::upgradeStarted);
        connect(fw, &ProtocolServiceFirmware::finished, this, &Nodes::upgradeFinished);*/
    }

    connect(this, &Fact::triggered, this, &Nodes::search);

    App::jsync(this);
}

int Nodes::nodesCount() const
{
    return m_nodesCount;
}

QVariant Nodes::data(int col, int role) const
{
    if (size() <= 0)
        return Fact::data(col, role);
    return NodesBase::data(col, role);
}

NodeItem *Nodes::addNode(const QString &sn, ProtocolNode *protocol)
{
    NodeItem *node = this->node(sn);
    if (node) {
        if (protocol)
            node->setProtocol(protocol);
        return node;
    }
    node = new NodeItem(this, sn, protocol);
    m_sn_map.insert(sn, node);
    m_nodesCount++;
    emit nodesCountChanged();
    return node;
}
void Nodes::removeNode(const QString &sn)
{
    if (m_protocol)
        m_protocol->removeNode(sn);
    NodeItem *node = this->node(sn);
    if (!node)
        return;
    node->remove();
    m_sn_map.remove(sn);
    m_nodesCount--;
    emit nodesCountChanged();
}

void Nodes::updateStatus()
{
    setValue(nodesCount() > 0 ? QString::number(nodesCount()) : "");
}
void Nodes::updateActions()
{
    bool enb = m_protocol;
    bool busy = progress() >= 0;
    bool upgrading = false; //model->isUpgrading();
    bool bModAll = modified();
    bool bEmpty = nodesCount() <= 0;
    f_search->setEnabled(enb);
    f_upload->setEnabled(enb && bModAll && (!(busy)));
    f_stop->setEnabled(enb && (busy || upgrading));
    f_reload->setEnabled(enb && (!(upgrading || bEmpty)));
    f_clear->setEnabled((!bEmpty) && (!(upgrading || busy)));
    f_status->setEnabled(enb && (!bEmpty) && (!(upgrading || busy)));
}

void Nodes::protocolFinished()
{
    if (vehicle->isLocal())
        return;

    int cnt = nodesCount();
    if (cnt <= 0)
        return;

    do {
        if (!(dictValid() && dataValid()))
            break;
        // exclude reconf nodes
        foreach (NodeItem *node, nodes()) {
            if (node->ident().flags.bits.reconf)
                cnt--;
        }
        bool bCntChanged = m_syncCount != cnt;
        m_syncCount = cnt;
        if (bCntChanged) {
            qDebug() << "sync" << vehicle->callsign()
                     << QString("%1 sec").arg(m_syncRequestTime.elapsed() / 1000.0, 0, 'f', 1);
            break;
        }

        // all valid and in sync
        qDebug() << "sync done";
        m_syncTimestamp = QDateTime::currentDateTimeUtc();
        setActive(false);
        emit syncDone();
        return;
    } while (0);

    //schedule re-sync
    syncLater(active() ? 100 : 1000);
}
void Nodes::sync()
{
    //qDebug() << syncActive;
    if (!m_protocol)
        return;
    if (vehicle->isTemporary())
        return;
    m_syncRequestTime.start();
    m_protocol->requestSearch();
}

void Nodes::syncLater(int timeout)
{
    if (m_syncTimer.isActive() && timeout > m_syncTimer.interval())
        timeout = m_syncTimer.interval();
    m_syncTimer.start(timeout);
    //qDebug() << timeout << vehicle->callsign();
}

void Nodes::search()
{
    if (!m_protocol)
        return;
    setActive(true);
    sync();
}
void Nodes::stop()
{
    if (!m_protocol)
        return;
    setActive(false);
    m_protocol->stop();
}

void Nodes::clear()
{
    if (upgrading())
        return;
    m_sn_map.clear();
    m_groups.clear();
    removeAll();
    if (m_protocol)
        m_protocol->clearNodes();
    m_syncCount = 0;
    m_nodesCount = 0;
    emit nodesCountChanged();
    setModified(false);
}

void Nodes::reload()
{
    clear();
    search();
}

void Nodes::upload()
{
    if (!(dictValid() && dataValid()))
        return;
    if (!modified())
        return;
    for (auto i : m_sn_map) {
        i->upload();
    }
}

void Nodes::save()
{
    if (!(dictValid() && dataValid()))
        return;
    for (auto i : m_sn_map) {
        if (!i->modified())
            continue;
        //storage->saveNodeConfig(node);
    }
}

void Nodes::rebootAll()
{
    if (!m_protocol)
        return;
    apxMsg() << tr("Vehicle system reset");
    m_protocol->requestRebootAll();
}

void Nodes::loadConfValue(const QString &sn, QString s)
{
    NodeItem *node = this->node(sn);
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
