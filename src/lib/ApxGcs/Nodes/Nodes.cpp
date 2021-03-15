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
#include "Nodes.h"
#include "NodeField.h"
#include "NodeItem.h"

#include <App/App.h>
#include <App/AppLog.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

Nodes::Nodes(Vehicle *vehicle, ProtocolNodes *protocol)
    : ProtocolViewBase(vehicle, protocol)
    , vehicle(vehicle)
{
    setOptions(FlatModel | ModifiedGroup | ProgressTrack);

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
    connect(f_status, &Fact::triggered, protocol, [protocol]() { protocol->requestStatus(); });

    //storage actions
    //TODO: f_lookup = new LookupConfigs(vehicle->protocol()->storage, this);

    f_save = new Fact(this, "save", tr("Save"), tr("Save configuration"), Action, "content-save");
    connect(f_save, &Fact::triggered, this, &Nodes::save);

    //FIXME: share f_share = new NodesShare(this, this);

    foreach (FactBase *a, actions()) {
        a->setOption(IconOnly);
        a->setOption(ShowDisabled);
    }

    connect(this, &Fact::modifiedChanged, this, &Nodes::updateActions);
    connect(protocol, &ProtocolNodes::enabledChanged, this, &Nodes::updateActions);
    connect(protocol, &ProtocolNodes::activeChanged, this, &Nodes::updateActions);
    connect(protocol, &ProtocolNodes::upgradingChanged, this, &Nodes::updateActions);
    connect(protocol, &ProtocolNodes::sizeChanged, this, &Nodes::updateActions);
    updateActions();

    connect(protocol, &ProtocolNodes::nodeNotify, this, &Nodes::nodeNotify);
    connect(protocol, &ProtocolNodes::syncDone, this, &Nodes::syncDone);

    // initial request
    if (vehicle->protocol()->isIdentified()) {
        protocol->requestSearch();
    }

    connect(this, &Fact::triggered, this, &Nodes::search);

    App::jsync(this);
}

void Nodes::nodeNotify(ProtocolNode *protocol)
{
    add(protocol);
}
NodeItem *Nodes::add(ProtocolNode *protocol)
{
    NodeItem *node = this->node(protocol->sn());
    if (node)
        return node;

    node = new NodeItem(this, this, protocol);
    m_sn_map.insert(protocol->sn(), node);

    return node;
}
void Nodes::syncDone()
{
    m_syncTimestamp = QDateTime::currentDateTimeUtc();
}

void Nodes::updateActions()
{
    bool enb = protocol()->enabled();
    bool upgrading = protocol()->upgrading();
    bool busy = protocol()->active() || upgrading;
    bool empty = protocol()->size() <= 0;
    //bool valid = protocol()->valid();
    bool mod = modified();
    f_search->setEnabled(enb);
    f_upload->setEnabled(enb && mod && !upgrading);
    f_stop->setEnabled(enb && busy);
    f_reload->setEnabled(enb && !upgrading);
    f_clear->setEnabled(!empty && !upgrading);
    f_status->setEnabled(enb && !empty && !upgrading);
}

void Nodes::search()
{
    if (!protocol()->enabled())
        return;
    protocol()->setActive(true);
    protocol()->requestSearch();
}
void Nodes::stop()
{
    qDebug() << sender();
    //vehicle->protocol()->vehicles->stopNmtRequests();
}

void Nodes::clear()
{
    if (protocol()->upgrading()) {
        apxMsgW() << tr("Upgrading in progress");
        return;
    }
    m_sn_map.clear();
    deleteChildren();
    protocol()->clear();
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
    //    if (!protocol()->valid())
    //        return;
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
    if (!protocol()->valid())
        return;

    //FIXME: save modified config
    /*for (auto i : m_sn_map) {
        if (!i->modified())
            continue;
        vehicle->protocol()->storage->saveNodeConfig(i->protocol());
    }*/

    //vehicle->protocol()->storage->saveConfiguration();
}

void Nodes::shell(QStringList commands)
{
    if (!commands.isEmpty()) {
        QString name = commands.first();
        NodeItem *n = nullptr;
        for (auto i : nodes()) {
            n = i;
            if (i->value().toString() == name)
                break;
            if (i->title() == name)
                break;
            if (QString("%1/%2").arg(i->title()).arg(i->value().toString()) == name)
                break;
            n = nullptr;
        }
        if (n) {
            n->shell(commands.mid(1));
            return;
        }
    }
    // fallback to all nodes
    for (auto i : nodes()) {
        i->shell(commands);
    }
}
