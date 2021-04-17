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
#include "NodeModules.h"
#include "NodeItem.h"
#include "Nodes.h"

NodeModules::NodeModules(Fact *parent, NodeItem *node, QString name)
    : Fact(parent, name.isEmpty() ? "modules" : name, "", "", Group)
    , _node(node)
    , _is_root(name.isEmpty())
{
    connect(this, &NodeModules::requestMod, node->protocol(), &PNode::requestMod);
    connect(this, &Fact::triggered, this, &NodeModules::update);

    auto f = new Fact(this, "refresh", tr("Refresh"), "", Action, "reload");
    connect(f, &Fact::triggered, this, &NodeModules::reload);

    if (_is_root) {
        setIcon("sitemap");
        setTitle(tr("Node modules"));
        setDescr(tr("Internal submodules status"));
        setDataType(Count);

        connect(node->protocol(), &PNode::modReceived, this, &NodeModules::modReceived);

        connect(node, &NodeItem::validChanged, this, &NodeModules::clear);
        return;
    }

    update();
}

void NodeModules::clear()
{
    deleteChildren();
    _done_ls = false;
}

void NodeModules::reload()
{
    update();
    for (auto i : facts()) {
        static_cast<NodeModules *>(i)->reload();
    }
}

QByteArray NodeModules::madr() const
{
    QByteArray adr;
    for (auto i = this; i && !i->_is_root; i = qobject_cast<NodeModules *>(i->parentFact())) {
        adr.prepend(static_cast<char>(i->num()));
    }
    return adr;
}

void NodeModules::update()
{
    //qDebug() << path();
    if (!_done_ls) {
        requestMod(PNode::ls, madr(), QStringList());
        return;
    }
    // request module status
    requestMod(PNode::status, madr(), QStringList());
}

void NodeModules::modReceived(PNode::mod_cmd_e cmd, QByteArray adr, QStringList data)
{
    // called in root module only
    auto m = findModule(adr);
    if (!m) {
        qWarning() << "missing module" << adr.toHex().toUpper() << data;
        return;
    }

    switch (cmd) {
    default:
        qWarning() << "unknown cmd" << cmd << m->path() << data;
        return;
    case PNode::ls:
        m->updateFacts(data);
        break;
    case PNode::status: {
        QString s = data.join('|').replace('\t', ' ').replace('\n', '|').simplified();
        if (m->size() > 0) {
            if (s.isEmpty())
                return;
            m->unbindProperties(m->child(0));
        }
        m->setValue(s);
        break;
    }
    }
}
NodeModules *NodeModules::findModule(QByteArray adr)
{
    if (adr.isEmpty())
        return this;
    auto n = static_cast<uint8_t>(adr.at(0));
    for (auto i : facts()) {
        auto m = qobject_cast<NodeModules *>(i);
        if (!m)
            continue;
        if (!n--)
            return m->findModule(adr.mid(1));
    }
    return {};
}

void NodeModules::updateFacts(QStringList names)
{
    if (_done_ls)
        return;
    _done_ls = true;

    if (names.isEmpty()) {
        setTreeType(NoFlags);
    } else {
        setTreeType(Group);
        for (auto s : names) {
            new NodeModules(this, _node, s);
        }
        if (size() > 0) {
            QStringList st;
            for (auto i : facts())
                st.append(i->name());
            setDescr(st.join(','));

            if (!_is_root) {
                auto f = child(0);
                bindProperty(f, "value", true);
            }
        }
    }

    // update values
    update();
}
