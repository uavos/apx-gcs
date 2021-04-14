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
        connect(_protocol, &PNodes::upgradingChanged, &_updateActions, &DelayedEvent::schedule);
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
    if (this->node(node->uid()))
        return;

    // register new online node
    auto f = new NodeItem(this, this, node);
    m_nodes.append(f);

    connect(f, &NodeItem::validChanged, this, &Nodes::updateValid);
    connect(f->storage,
            &NodeStorage::configSaved,
            vehicle->storage(),
            &VehicleStorage::saveVehicleConfig);

    updateValid();
    updateActions();
    nodeNotify(f);
}
void Nodes::syncDone()
{
    m_syncTimestamp = QDateTime::currentDateTimeUtc();
}

void Nodes::updateActions()
{
    bool enb = _protocol;
    bool upg = upgrading();
    bool bsy = busy();
    bool empty = nodes().size() <= 0;
    bool mod = modified();
    f_search->setEnabled(enb);
    f_upload->setEnabled(enb && mod && !upg);
    f_stop->setEnabled(enb && bsy);
    f_reload->setEnabled(enb && !upg);
    f_clear->setEnabled(!empty && !upg);
    f_status->setEnabled(enb && !empty && !upg);
}

bool Nodes::upgrading() const
{
    return _protocol && _protocol->upgrading();
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
    if (!_protocol)
        return;

    _protocol->root()->cancelRequests();
}

void Nodes::clear()
{
    if (upgrading()) {
        apxMsgW() << tr("Upgrading in progress");
        return;
    }
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
    if (vehicle->isReplay())
        return;

    if (upgrading()) {
        apxMsgW() << tr("Upgrading in progress");
        return;
    }

    stop();
    clear();
    search();
}

void Nodes::upload()
{
    if (!_protocol)
        return;

    if (!modified())
        return;
    for (auto i : nodes()) {
        i->upload();
    }
}

void Nodes::shell(QStringList commands)
{
    if (!_protocol)
        return;

    if (commands.size() > 1) {
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
        apxMsgW() << tr("Inconsistent nodes");
        return;
    }

    // imprt by UID
    QList<NodeItem *> nlist = m_nodes;
    QVariantList vlist;
    for (auto i : nodes) {
        auto node = i.value<QVariantMap>();
        auto uid = node.value("info").value<QVariantMap>().value("uid").toString();
        NodeItem *n = this->node(uid);
        if (n) {
            n->fromVariant(node);
            nlist.removeOne(n);
        } else {
            vlist.append(node);
        }
    }
    apxMsg() << tr("Configuration loaded for %1 nodes").arg(m_nodes.size());

    do {
        if (vlist.isEmpty() || nlist.isEmpty())
            break;

        // try to import by guessing nodes
        apxMsgW() << tr("Importing %1 nodes").arg(vlist.size()).append("...");

        // consume vlist and nlist items

        // find by name-label match
        for (int i = 0; i < vlist.size(); ++i) {
            if (nlist.isEmpty())
                break;
            auto node = vlist.at(i).value<QVariantMap>();
            auto info = node.value("info").value<QVariantMap>();
            auto name = info.value("name").toString();
            auto label = node.value("values").value<QVariantMap>().value("label").toString();

            for (auto n : nlist) {
                if (n->title() != name && n->label() != label)
                    continue;
                n->fromVariant(node);
                nlist.removeOne(n);
                vlist.removeAt(i);
                break;
            }
        }
        if (vlist.isEmpty() || nlist.isEmpty())
            break;

        // find by name match
        for (int i = 0; i < vlist.size(); ++i) {
            if (nlist.isEmpty())
                break;
            auto node = vlist.at(i).value<QVariantMap>();
            auto info = node.value("info").value<QVariantMap>();
            auto name = info.value("name").toString();
            auto label = node.value("values").value<QVariantMap>().value("label").toString();

            for (auto n : nlist) {
                if (n->title() != name)
                    continue;
                n->fromVariant(node);
                nlist.removeOne(n);
                vlist.removeAt(i);
                break;
            }
        }
        if (vlist.isEmpty() || nlist.isEmpty())
            break;

        // try import shiva config
        do {
            QVariantList shivas; // find import shivas
            for (int i = 0; i < vlist.size(); ++i) {
                auto node = vlist.at(i).value<QVariantMap>();
                auto sh = node.value("values").value<QVariantMap>().value("shiva.mode");
                if (!sh.toBool())
                    continue;
                // enabled shivas only
                shivas.append(node); // enables shiva only
            }
            if (shivas.isEmpty())
                break;
            QMap<QString, NodeItem *> nshiva;
            // find available to import shivas with priority to enabled ones
            for (auto n : nlist) {
                auto mode = n->findChild("shiva.mode");
                if (!mode)
                    continue; // no shiva in node config
                auto on = mode->value().toUInt();
                auto n_prev = nshiva.value(n->title());
                if (n_prev) {
                    auto on_prev = n_prev->findChild("shiva.mode")->value().toUInt();
                    if (on_prev > 0 && on_prev < on)
                        continue; // mode priority is higher
                }
                nshiva.insert(n->title(), n);
            }
            if (nshiva.isEmpty())
                break;

            for (auto shiva : shivas) {
                // look for nodes by priority
                NodeItem *n = {};
                // search for enabled nav
                n = nshiva.value("nav");
                if (n) {
                    auto on = n->findChild("shiva.mode")->value().toUInt();
                    if (!on)
                        n = {};
                }
                if (!n) { // search for any enabled
                    for (auto i : nshiva.values()) {
                        bool on = i->findChild("shiva.mode")->value().toUInt();
                        if (!on)
                            continue;
                        n = i;
                        break;
                    }
                }
                if (!n) { // search for any nav
                    n = nshiva.value("nav");
                }
                if (!n) { // search for any available
                    n = nshiva.values().first();
                }
                if (!n)
                    continue;

                apxMsg() << tr("Importing autopilot config for %1").arg(n->title());

                n->fromVariant(shiva);
                nlist.removeOne(n);
                vlist.removeOne(shiva);
            }
        } while (0); // shiva import

    } while (0);

    if (!vlist.isEmpty()) {
        apxMsg() << tr("Ignored configuration for %1 nodes").arg(vlist.size());
        QStringList st;
        for (auto i : vlist) {
            auto node = i.value<QVariantMap>();
            auto info = node.value("info").value<QVariantMap>();
            auto name = info.value("name").toString();
            auto label = node.value("values").value<QVariantMap>().value("label").toString();
            QString s = name;
            if (!label.isEmpty())
                s.append(QString("-%1").arg(label));
            st.append(s);
        }
        apxMsg() << tr("Ignored import nodes: %1").arg(st.join(','));
    }
    if (!nlist.isEmpty()) {
        apxMsg() << tr("Missing configuration for %1 nodes").arg(nlist.size());
        QStringList st;
        for (auto i : nlist) {
            QString s = i->title();
            if (!i->label().isEmpty())
                s.append(QString("-%1").arg(i->label()));
            st.append(s);
        }
        apxMsg() << tr("Extra nodes: %1").arg(st.join(','));
    }
}
