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
#include "NodeTools.h"
#include "NodeItem.h"
#include "Nodes.h"
#include <App/App.h>
#include <Database/Database.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
NodeTools::NodeTools(NodeItem *anode, Flags flags)
    : NodeToolsGroup(anode, anode, "tools", tr("Tools"), tr("Node tools"), flags | FlatModel)
{
    setIcon("wrench");

    QString sect = tr("Backups");

    f_backups = new LookupNodeBackup(node, this);
    f_backups->setSection(sect);

    f_restore = new Fact(this, "recent", tr("Restore recent"), tr("Restore the most recent backup"));
    f_restore->setIcon("undo");
    f_restore->setSection(sect);
    connect(f_restore, &Fact::triggered, node->nodes->storage, [this]() {
        this->node->nodes->storage->restoreNodeConfig(this->node);
    });

    //sections
    f_cmd = new NodeToolsGroup(this,
                               node,
                               "cmd",
                               tr("Commands"),
                               tr("Node hardware commands"),
                               Section);
    f_syscmd = new NodeToolsGroup(this,
                                  node,
                                  "syscmd",
                                  tr("System"),
                                  tr("System hardware commands"),
                                  Section);
    f_maintenance = new NodeToolsGroup(this,
                                       node,
                                       "maintenance",
                                       tr("Maintenance"),
                                       tr("Hardware maintenance"),
                                       Group);
    f_maintenance->setSection(f_syscmd->title());
    f_maintenance->setIcon("wrench");

    //maintenance
    f_updates = new Fact(f_maintenance, "updates", tr("Updates"), tr("Firmware updates"), Group);
    f_updates->setIcon("chip");

    Fact *f;
    f = new Fact(f_updates, "firmware", tr("Firmware"), tr("Update node firmware"));
    connect(f, &Fact::triggered, node, &NodeItem::upgradeFirmware);
    f = new Fact(f_updates, "loader", tr("Loader"), tr("Update node loader"));
    connect(f, &Fact::triggered, node, &NodeItem::upgradeLoader);
    if (node->title() == "mhx") {
        f = new Fact(f_updates, "radio", tr("Radio"), tr("Update radio module"));
        connect(f, &Fact::triggered, node, &NodeItem::upgradeRadio);
    }

    f_rebootall = new Fact(f_maintenance, "rebootall", tr("Reboot all"), tr("Vehicle system reset"));
    f_rebootall->setIcon("reload");
    connect(f_rebootall, &Fact::triggered, node->nodes, [=]() { node->nodes->rebootAll(); });

    registerOnlineAction(f_updates);
}
//=============================================================================
Fact *NodeTools::addCommand(QString name, QString title, QString descr, quint16 cmd)
{
    bool sys = cmd < 128;
    //qDebug()<<node->title()<<name<<descr<<sys;
    NodeToolsGroup *fp = sys ? f_syscmd : f_cmd;
    Fact *f = fp->addCommand(name, title, descr, cmd);
    if (!f)
        return f;

    connect(f, &Fact::triggered, node, [this, f]() {
        node->execCommand(static_cast<quint16>(f->userData.toUInt()), f->name(), f->title());
    });

    return f;
}
void NodeTools::clearCommands()
{
    f_cmd->removeAll();
    f_syscmd->removeAll();
}
//=============================================================================
