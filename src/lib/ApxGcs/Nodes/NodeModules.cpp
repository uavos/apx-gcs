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
        return;
    }

    update();
}

void NodeModules::reload()
{
    update();
    for (auto i : facts()) {
        static_cast<NodeModules *>(i)->reload();
    }
}

QStringList NodeModules::mpath() const
{
    QStringList st;
    for (auto i = this; i && !i->_is_root; i = qobject_cast<NodeModules *>(i->parentFact())) {
        st.prepend(i->name());
    }
    return st;
}

void NodeModules::update()
{
    //qDebug() << path();
    if (!_done_ls) {
        QStringList data;
        data.append("ls");
        data.append(mpath());
        requestMod(data);
        return;
    }
    // request module status
    QStringList data;
    data.append("status");
    data.append(mpath());
    requestMod(data);
}

void NodeModules::modReceived(QStringList data)
{
    if (data.isEmpty())
        return;

    auto cmd = data.takeFirst();

    // called in root module only
    auto mpath = data.mid(0, data.indexOf(":"));
    auto reply = data.mid(mpath.size() + 1);

    auto m = mpath.isEmpty() ? this : qobject_cast<NodeModules *>(findChild(mpath.join('.')));
    if (!m) {
        qWarning() << "missing fact" << mpath << data;
        return;
    }

    if (cmd == "ls") {
        m->updateFacts(reply);
        return;
    }

    if (cmd == "status") {
        QString s = reply.join('|').replace('\t', ' ').replace('\n', '|').simplified();
        if (m->size() > 0) {
            if (s.isEmpty())
                return;
            m->unbindProperties(m->child(0));
        }
        m->setValue(s);
        return;
    }
}

void NodeModules::updateFacts(QStringList names)
{
    if (names.isEmpty()) {
        setTreeType(NoFlags);
    } else {
        for (auto s : names) {
            if (child(s))
                continue;
            new NodeModules(this, _node, s);
        }
        if (size() > 0 && !_is_root) {
            auto f = child(0);
            setDescr(f->name());
            bindProperty(f, "value", true);
        }
    }
    if (_done_ls)
        return;
    _done_ls = true;
    // update values
    update();
}
