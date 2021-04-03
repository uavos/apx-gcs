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

#include <algorithm>

// #include "LookupConfigs.h"

Nodes::Nodes(Vehicle *vehicle)
    : Fact(vehicle,
           "nodes",
           tr("Nodes"),
           tr("Vehicle components"),
           Group | FlatModel | ModifiedGroup | ProgressTrack)
    , vehicle(vehicle)
    , _protocol(vehicle->protocol() ? vehicle->protocol()->nodes() : nullptr)
{
    setIcon("puzzle");

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
    //connect(f_status, &Fact::triggered, protocol, [protocol]() { protocol->requestStatus(); });

    for (auto a : actions()) {
        a->setOption(IconOnly);
        a->setOption(ShowDisabled);
    }

    connect(&_updateActions, &DelayedEvent::triggered, this, &Nodes::updateActions);
    connect(this, &Fact::modifiedChanged, &_updateActions, &DelayedEvent::schedule);
    if (_protocol) {
        connect(_protocol, &PNodes::busyChanged, &_updateActions, &DelayedEvent::schedule);
        connect(_protocol, &PNodes::node_available, this, &Nodes::node_available);
    }

    updateActions();

    if (_protocol) {
        bindProperty(_protocol, "value", true);
    }

    connect(this, &Fact::triggered, this, &Nodes::search);

    if (vehicle->isIdentified())
        _protocol->requestSearch();
}

NodeItem *Nodes::node(const QString &uid) const
{
    for (auto i : nodes()) {
        if (i->uid() == uid)
            return i;
    }
    return nullptr;
}

void Nodes::node_available(PNode *node)
{
    add(node);
}
NodeItem *Nodes::add(PNode *protocol)
{
    NodeItem *node = this->node(protocol->uid());
    if (node)
        return node;

    node = new NodeItem(this, this, protocol);
    m_nodes.append(node);

    connect(node, &NodeItem::validChanged, this, &Nodes::updateValid);
    connect(node->storage,
            &NodeStorage::configSaved,
            vehicle->storage(),
            &VehicleStorage::saveVehicleConfig);

    updateValid();
    updateActions();
    return node;
}
void Nodes::syncDone()
{
    m_syncTimestamp = QDateTime::currentDateTimeUtc();
}

void Nodes::updateActions()
{
    bool enb = true;        //protocol()->enabled();
    bool upgrading = false; //protocol()->upgrading();
    bool busy = _protocol ? _protocol->busy() || upgrading : false;
    bool empty = nodes().size() <= 0;
    //bool valid = protocol()->valid();
    bool mod = modified();
    f_search->setEnabled(enb);
    f_upload->setEnabled(enb && mod && !upgrading);
    f_stop->setEnabled(enb && busy);
    f_reload->setEnabled(enb && !upgrading);
    f_clear->setEnabled(!empty && !upgrading);
    f_status->setEnabled(enb && !empty && !upgrading);
}

void Nodes::updateValid()
{
    bool v = !nodes().isEmpty();
    for (auto i : m_nodes) {
        if (i->valid())
            continue;
        v = false;
        break;
    }
    if (m_valid == v)
        return;
    m_valid = v;
    emit validChanged();

    if (!m_valid)
        return;
    qDebug() << "nodes valid" << vehicle->title();
}

void Nodes::search()
{
    if (!_protocol)
        return;
    _protocol->requestSearch();
}
void Nodes::stop()
{
    //qDebug() << sender();
    _protocol->cancelRequests();
    //vehicle->protocol()->vehicles->stopNmtRequests();
}

void Nodes::clear()
{
    /*if (protocol()->upgrading()) {
        apxMsgW() << tr("Upgrading in progress");
        return;
    }*/
    if (m_valid) {
        m_valid = false;
        emit validChanged();
    }
    m_nodes.clear();
    deleteChildren();
    setModified(false);
}

void Nodes::reload()
{
    clear();
    search();
}

void Nodes::upload()
{
    if (!modified())
        return;
    for (auto i : nodes()) {
        i->upload();
    }
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

QString Nodes::getConfigTitle()
{
    QMap<QString, QString> map, shiva;

    for (auto node : nodes()) {
        auto s = node->valueText();
        if (s.isEmpty())
            continue;
        map.insert(node->title(), s);
        for (auto field : node->fields()) {
            if (field->name() != "shiva")
                continue;
            shiva.insert(node->title(), s);
            break;
        }
    }
    if (map.isEmpty())
        return {};
    if (!shiva.isEmpty())
        map = shiva;

    QString s;
    s = map.value("nav");
    if (!s.isEmpty())
        return s;
    s = map.value("com");
    if (!s.isEmpty())
        return s;
    s = map.value("ifc");
    if (!s.isEmpty())
        return s;

    auto st = map.values();
    std::sort(st.begin(), st.end(), [](const auto &s1, const auto &s2) {
        return s1.size() > s2.size();
    });
    return st.first();
}

QVariant Nodes::toVariant() const
{
    QVariantList list;
    for (auto i : nodes()) {
        list.append(i->toVariant());
    }
    return list;
}
void Nodes::fromVariant(const QVariant &var)
{
    auto nodes = var.value<QVariantList>();
    if (nodes.isEmpty()) {
        return;
    }

    if (vehicle->isReplay()) {
        // check if nodes set is the same
        size_t match_cnt = 0;
        for (auto i : nodes) {
            auto uid
                = i.value<QVariantMap>().value("info").value<QVariantMap>().value("uid").toString();
            if (this->node(uid))
                match_cnt++;
        }

        if (match_cnt != m_nodes.size())
            clear();

        for (auto i : nodes) {
            auto node = i.value<QVariantMap>();
            auto uid = node.value("info").value<QVariantMap>().value("uid").toString();
            NodeItem *f = this->node(uid);
            if (!f) {
                f = new NodeItem(this, this, nullptr);
                m_nodes.append(f);
            }
            f->fromVariant(node);
        }

        updateValid();
        updateActions();
        return;
    }

    // import to existing nodes
    if (!valid()) {
        apxConsoleW() << tr("Inconsistent nodes");
        return;
    }

    // imprt by UID
    QList<NodeItem *> nlist = m_nodes;
    QVariantList vlist;
    for (auto i : nodes) {
        auto node = i.value<QVariantMap>();
        auto uid = node.value("info").value<QVariantMap>().value("uid").toString();
        NodeItem *f = this->node(uid);
        if (f) {
            f->fromVariant(node);
            nlist.removeOne(f);
        } else {
            vlist.append(node);
        }
    }
    apxConsole() << tr("Configuration loaded for %1 nodes").arg(m_nodes.size());
    if (vlist.isEmpty() && nlist.isEmpty()) {
        return;
    }
    // try to import by guessing nodes
    apxConsoleW() << tr("Importing %1 nodes").arg(vlist.size());
    // TODO: import nodes substitutions
}
