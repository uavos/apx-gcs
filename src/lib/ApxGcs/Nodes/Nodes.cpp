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

Nodes::Nodes(Vehicle *vehicle, ProtocolNodes *protocol)
    : ProtocolViewBase(vehicle, protocol)
    , vehicle(vehicle)
{
    setTitle(tr("Nodes"));
    setDescr(tr("Vehicle components"));
    setOptions(FlatModel | ModifiedGroup);

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
    //connect(f_status, &Fact::triggered, this, &Nodes::status);

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
    connect(protocol, &ProtocolNodes::nodeFound, this, &Nodes::nodeFound);
    connect(protocol, &ProtocolNodes::dataExchangeFinished, this, &Nodes::dataExchangeFinished);

    connect(this, &Fact::activeChanged, protocol, [this]() {
        this->protocol()->setActive(active());
    });

    if (!(vehicle->isLocal() || vehicle->isReplay())) {
        protocol->requestSearch();
    }

    /*ProtocolServiceFirmware *fw = Vehicles::instance()->protocol->firmware;
        connect(fw, &ProtocolServiceFirmware::started, this, &Nodes::upgradeStarted);
        connect(fw, &ProtocolServiceFirmware::finished, this, &Nodes::upgradeFinished);*/

    connect(this, &Fact::triggered, this, &Nodes::search);

    App::jsync(this);
}

bool Nodes::check_valid() const
{
    for (auto const i : m_sn_map) {
        if (!i->dictValid())
            return false;
        if (!i->dataValid())
            return false;
    }
    return true;
}

int Nodes::nodesCount() const
{
    return m_sn_map.size();
}

void Nodes::nodeFound(ProtocolNode *protocol)
{
    add(protocol);
}
NodeItem *Nodes::add(ProtocolNode *protocol)
{
    NodeItem *node = this->node(protocol->sn());
    if (node)
        return node;

    node = new NodeItem(this, protocol);
    m_sn_map.insert(protocol->sn(), node);
    emit nodesCountChanged();
    return node;
}

void Nodes::updateStatus()
{
    setValue(nodesCount() > 0 ? QString::number(nodesCount()) : "");
}
void Nodes::updateActions()
{
    bool enb = protocol()->enabled();
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

void Nodes::dataExchangeFinished()
{
    if (vehicle->isLocal())
        return;
    if (vehicle->isReplay())
        return;

    int cnt = nodesCount();
    if (cnt <= 0)
        return;

    do {
        if (!check_valid())
            break;
        // exclude reconf nodes
        foreach (NodeItem *node, nodes()) {
            if (node->ident().flags.bits.reconf)
                cnt--;
        }
        bool bCntChanged = m_syncCount != cnt;
        m_syncCount = cnt;
        if (bCntChanged) {
            qDebug() << "sync" << vehicle->title()
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
    m_syncRequestTime.start();
    protocol()->requestSearch();
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
    if (!protocol()->enabled())
        return;
    setActive(true);
    sync();
}
void Nodes::stop()
{
    setActive(false);
}

void Nodes::clear()
{
    m_sn_map.clear();
    m_groups.clear();
    removeAll();
    protocol()->clearNodes();
    m_syncCount = 0;
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
    if (!protocol()->enabled())
        return;
    if (!check_valid())
        return;
    if (!modified())
        return;
    for (auto i : m_sn_map) {
        i->upload();
    }
}

void Nodes::save()
{
    if (!protocol()->enabled())
        return;
    if (!check_valid())
        return;
    for (auto i : m_sn_map) {
        if (!i->modified())
            continue;
        //storage->saveNodeConfig(node);
    }
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
    //FIXME: node->loadConfigValue(spath, sv);
}
