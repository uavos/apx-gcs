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

NodeModules::NodeModules(NodeItem *node, NodeModules *parent, QString name)
    : Fact()
    , _node(node)
{
    if (parent)
        setParentFact(parent);
    else
        setParentFact(node);

    setName(name.isEmpty() ? "modules" : name);

    setFlags(Group | Count);

    connect(this, &NodeModules::requestMod, node->protocol(), &PNode::requestMod);
    connect(this, &Fact::triggered, this, &NodeModules::update);

    if (is_root()) {
        setTitle(tr("modules"));
        setDescr(tr("Node modules"));

        connect(node->protocol(), &PNode::modReceived, this, &NodeModules::modReceived);
        return;
    }

    QTimer::singleShot(100 * num() + 100, this, &NodeModules::update);
}

bool NodeModules::is_root() const
{
    return parentFact() == _node;
}
QStringList NodeModules::mpath() const
{
    QStringList st;
    for (auto i = this; i && !i->is_root(); i = qobject_cast<NodeModules *>(i->parentFact())) {
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
        QString s = reply.join(',').replace('\t', ' ');
        if (s.isEmpty())
            return;
        setDataType(NoFlags);
        m->setValue(s);
        return;
    }
}

void NodeModules::updateFacts(QStringList names)
{
    if (names.isEmpty()) {
        setFlags(NoFlags);
    } else {
        for (auto s : names) {
            if (child(s))
                continue;
            new NodeModules(_node, this, s);
        }
        if (size() == 1) {
            auto f = child(0);
            setDataType(NoFlags);
            // setTitle(QString("%1/%2").arg(name()).arg(f->name()));
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
