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
//=============================================================================
NodeToolsGroup::NodeToolsGroup(Fact *parent,
                               NodeItem *node,
                               const QString &name,
                               const QString &title,
                               const QString &descr,
                               Flags flags)
    : Fact(parent, name.toLower(), title, descr, flags)
    , node(node)
{}
//=============================================================================
Fact *NodeToolsGroup::addCommand(QString name, QString title, QString descr, quint16 cmd)
{
    name = name.toLower();
    if (title.contains(':')) {
        //recursive grouping
        QString sgroup = title.left(title.indexOf(':')).trimmed();
        title = title.remove(0, title.indexOf(':') + 1).trimmed();
        NodeToolsGroup *fg = nullptr;
        for (int i = 0; i < size(); ++i) {
            NodeToolsGroup *g = qobject_cast<NodeToolsGroup *>(child(i));
            if (!g)
                continue;
            if (g->title() == sgroup) {
                fg = g;
                break;
            }
        }
        if (!fg) {
            fg = new NodeToolsGroup(this, node, sgroup, sgroup, "");
            if (name.startsWith("bb"))
                fg->setIcon("database");
            if (name.startsWith("vm"))
                fg->setIcon("code-braces");
            registerOnlineAction(fg);
        }
        return fg->addCommand(name, title, descr, cmd);
    }
    Fact *f = new Fact(this, name, title, descr);
    if (name.contains("reboot") || name.contains("restart"))
        f->setIcon("reload");
    else if (name.contains("mute"))
        f->setIcon("volume-mute");
    else if (name.contains("erase") || name.contains("clear"))
        f->setIcon("close-circle");
    else if (name.contains("conf"))
        f->setIcon("alert-octagram");
    else if (name.startsWith("vm"))
        f->setIcon("code-braces");
    else
        f->setIcon("asterisk");
    f->userData = cmd;
    registerOnlineAction(f);
    //App::jsync(this);
    return f;
}
//=============================================================================
void NodeToolsGroup::registerOnlineAction(Fact *fact)
{
    connect(node, &NodeItem::offlineChanged, fact, [fact, this]() {
        fact->setEnabled(!node->offline());
    });
    fact->setEnabled(!node->offline());
}
//=============================================================================
