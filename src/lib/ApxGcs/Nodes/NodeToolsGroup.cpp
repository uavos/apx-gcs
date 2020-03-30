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
#include "NodeToolsGroup.h"
#include "NodeItem.h"

NodeToolsGroup::NodeToolsGroup(Fact *parent,
                               NodeItem *node,
                               const QString &name,
                               const QString &title,
                               const QString &descr,
                               Flags flags)
    : Fact(parent, name.toLower(), title, descr, flags)
    , node(node)
{
    if (name == "bb")
        setIcon("database");
    if (name == "vm")
        setIcon("code-braces");
    registerOnlineAction(this);
}

Fact *NodeToolsGroup::addCommand(Fact *group,
                                 QString name,
                                 QString title,
                                 xbus::node::usr::cmd_t cmd)
{
    name = name.toLower();
    if (group != node) {
        // create groups
        FactList plist;
        while (group != node) {
            plist.insert(0, group);
            group = group->parentFact();
        }
        group = this;
        for (auto i : plist) {
            NodeToolsGroup *g = qobject_cast<NodeToolsGroup *>(group->child(i->name()));
            if (g)
                group = g;
            else
                group = new NodeToolsGroup(group, node, i->name(), i->title(), i->descr());
        }
    } else
        group = this;

    Fact *f = new Fact(group, name, title);
    if (name.contains("reboot") || name.contains("restart"))
        f->setIcon("reload");
    else if (name.contains("mute"))
        f->setIcon("volume-mute");
    else if (name.contains("erase") || name.contains("clear"))
        f->setIcon("close-circle");
    else if (name.contains("conf"))
        f->setIcon("alert-octagram");
    else if (group->name() == "vm")
        f->setIcon("code-braces");
    else
        f->setIcon("asterisk");
    f->userData = cmd;
    registerOnlineAction(f);
    //App::jsync(this);
    return f;
}

void NodeToolsGroup::registerOnlineAction(Fact *fact)
{
    fact->bindProperty(node->protocol(), "enabled");
}
