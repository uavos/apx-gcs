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
#include "Blackbox.h"
#include "BlackboxFile.h"
#include "BlackboxNode.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <Fact/Fact.h>
#include <Fleet/Fleet.h>
#include <Nodes/NodeItem.h>

Blackbox::Blackbox(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Blackbox"),
           tr("Blackbox data downloader"),
           Group,
           "dropbox")
{
    connect(Fleet::instance(), &Fleet::nodeNotify, this, &Blackbox::nodeNotify);

    f_import = new BlackboxFile(this);
}

void Blackbox::nodeNotify(NodeItem *node)
{
    if (!node->dataValid())
        return;
    // check if already known
    for (int i = 0; i < size(); ++i) {
        Fact *f = child(i);
        if (f->bind() == node)
            return;
    }
    BlackboxNode *bb = nullptr;
    //check if have blackbox commands
    for (int i = 0; i < node->tools->f_cmd->size(); ++i) {
        Fact *g = node->tools->f_cmd->child(i);
        for (int j = 0; j < g->size(); ++j) {
            Fact *c = g->child(j);
            if (!c->name().startsWith("bb_"))
                continue;
            if (!bb) {
                bb = new BlackboxNode(this, node);
            }
            bb->addCommand(c);
        }
    }
}
