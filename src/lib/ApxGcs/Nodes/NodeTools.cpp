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
#include <Database/Database.h>
#include <ApxApp.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
NodeTools::NodeTools(NodeItem *parent)
    : Fact(nullptr, "tools", tr("Tools"), tr("Node tools"), Group)
    , node(parent)
{
    setIcon("wrench");
    setParent(parent);
    connect(parent, &Fact::removed, this, &Fact::removed);

    model()->setFlat(true);

    QString sect = tr("Backups");

    f_backups = new LookupNodeBackup(node, this);
    f_backups->setSection(sect);

    f_restore = new Fact(this, "recent", tr("Restore recent"), tr("Restore the most recent backup"));
    f_restore->setIcon("undo");
    f_restore->setSection(sect);
    connect(f_restore, &Fact::triggered, node->nodes->storage, [this]() {
        node->nodes->storage->restoreNodeConfig(node);
    });

    //sections
    f_cmd = new Fact(this, "cmd", tr("Commands"), tr("Node hardware commands"), Section);
    f_syscmd = new Fact(this, "syscmd", tr("System"), tr("System hardware commands"), Section);
    f_maintenance = new Fact(this,
                             "maintenance",
                             tr("Maintenance"),
                             tr("Hardware maintenance"),
                             Section);

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

    onlineActions.append(f_updates);
    connect(node, &NodeItem::offlineChanged, this, &NodeTools::updateActions);
    updateActions();
}
//=============================================================================
void NodeTools::addCommand(DictNode::Command cmd)
{
    commands.append(cmd);
    bool sys = cmd.cmd < 128;
    //qDebug()<<node->title()<<name<<descr<<sys;
    QString name = cmd.name.toLower();
    QString descr = cmd.descr;
    Fact *fp = sys ? f_syscmd : f_cmd;
    if (descr.contains(':')) {
        //grouping
        QString sgroup = descr.left(descr.indexOf(':')).trimmed();
        descr = descr.remove(0, descr.indexOf(':') + 1).trimmed();
        Fact *fgroup = nullptr;
        for (int i = 0; i < fp->size(); ++i) {
            if (fp->child(i)->title() != sgroup)
                continue;
            fgroup = fp->child(i);
            break;
        }
        if (!fgroup) {
            fgroup = new Fact(fp, sgroup, sgroup, "", Group);
            onlineActions.append(fgroup);
        }
        fp = fgroup;
    }
    Fact *f = new Fact(fp, name, descr, "");
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
    else if (name.startsWith("bb"))
        f->setIcon("database");
    else
        f->setIcon("asterisk");
    f->userData = cmd.cmd;
    connect(f, &Fact::triggered, node, [this, f]() {
        node->execCommand(static_cast<quint16>(f->userData.toUInt()), f->name(), f->title());
    });
    onlineActions.append(f);
    updateActions();
    //ApxApp::jsync(this);
}
void NodeTools::clearCommands()
{
    onlineActions.clear();
    commands.clear();
    f_cmd->removeAll();
    f_syscmd->removeAll();
}
//=============================================================================
void NodeTools::updateActions()
{
    bool enb = !node->offline();
    foreach (Fact *f, onlineActions) {
        f->setEnabled(enb);
    }
}
//=============================================================================
