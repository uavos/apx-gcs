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
#include "NodeTools.h"
#include "NodeItem.h"
#include "NodeModules.h"
#include "Nodes.h"

#include <App/App.h>
#include <Database/Database.h>
#include <Fleet/Fleet.h>

NodeTools::NodeTools(NodeItem *anode, Flags flags)
    : NodeToolsGroup(anode, anode, "tools", tr("Tools"), tr("Node tools"), flags | FlatModel)
{
    setIcon("wrench");
    Fact *f;

    QString sect = tr("Backups");

    f_storage = new NodeStorage(node, this);
    f_storage->setSection(sect);

    f_restore = new Fact(this, "recent", tr("Restore recent"), tr("Restore the most recent backup"));
    f_restore->setIcon("undo");
    f_restore->setSection(sect);
    connect(f_restore, &Fact::triggered, f_storage, &NodeStorage::loadLatestNodeConf);

    //sections
    f_usr = new NodeToolsGroup(this,
                               node,
                               "usr",
                               tr("Commands"),
                               tr("Node hardware commands"),
                               Section);

    // maintenance
    f_maintenance = new NodeToolsGroup(this,
                                       node,
                                       "maintenance",
                                       tr("Maintenance"),
                                       tr("Hardware maintenance"),
                                       Group | FlatModel);
    f_maintenance->setIcon("wrench");

    f_updates = new Fact(f_maintenance, "updates", tr("Updates"), tr("Firmware updates"), Group);
    f_updates->setIcon("chip");

    f = new Fact(f_updates, "firmware", tr("Firmware"), tr("Update node firmware"));
    connect(f, &Fact::triggered, this, [this]() { node->nodes()->requestUpgrade(node, "fw"); });

    f = new Fact(f_updates, "loader", tr("Loader"), tr("Update node loader"));
    connect(f, &Fact::triggered, this, [this]() { node->nodes()->requestUpgrade(node, "ldr"); });

    registerOnlineAction(f_updates);

    // status requests
    /*f_status = new NodeToolsGroup(f_maintenance,
                                  node,
                                  "status",
                                  tr("Status"),
                                  tr("Node status request"),
                                  Group);
    f_status->setIcon("playlist-check");*/
    //TODO stats node command menu
    /*f = new Fact(f_status, "stats", tr("Statistics"), tr("Request counters"));
    connect(f, &Fact::triggered, this, [this]() { node->protocol()->requestStatus(); });
    f = new Fact(f_status, "mem", tr("Memory"), tr("Request memory usage"));
    connect(f, &Fact::triggered, this, [this]() { node->shell(QStringList() << "tasks"); });*/
    //registerOnlineAction(f_status);

    f_sys = new NodeToolsGroup(f_maintenance,
                               node,
                               "sys",
                               tr("System"),
                               tr("System commands"),
                               Section);

    f_maintenance->setSection(f_sys->title());

    // online tools
    if (node->protocol()) {
        f = new NodeModules(this, node);
        f->setSection(f_sys->title());

        f = new Fact(f_maintenance, "reboot", tr("Reboot node"), tr("Node system reset"));
        f->setIcon("reload");
        f->setSection(f_sys->title());
        connect(f, &Fact::triggered, this, [this, f]() {
            node->message(f->title().append(": ").append(node->title()));
            node->protocol()->requestReboot();
        });
    }
}

Fact *NodeTools::addCommand(Fact *group, QString name, QString title, xbus::node::usr::cmd_t cmd)
{
    //qDebug() << node->title() << group << name << title << cmd;

    NodeToolsGroup *g = f_usr;
    if (group->name() == f_sys->name()) {
        g = f_sys;
        group = node;
    }
    Fact *f = g->addCommand(group, name, title, cmd);
    if (!f)
        return f;

    connect(f, &Fact::triggered, this, [this, f]() { execUsr(f); });

    return f;
}

void NodeTools::clearCommands()
{
    f_usr->deleteChildren();
    f_sys->deleteChildren();
}

void NodeTools::execUsr(Fact *f)
{
    QString msg = tr("User command").append(": ").append(f->title());
    if (!f->descr().isEmpty())
        msg.append(QString(" (%1)").arg(f->descr()));

    if (!node->protocol()) {
        msg.append(QString(" (%1)").arg(tr("ignored")));
        node->message(msg);
        return;
    }

    node->message(msg);
    node->protocol()->requestUsr(static_cast<quint8>(f->property("cmd").toUInt()), QByteArray());
}
