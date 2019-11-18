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
#include "Blackbox.h"
#include "BlackboxFile.h"
#include "BlackboxNode.h"

#include <App/App.h>
#include <App/AppDirs.h>
#include <Fact/Fact.h>
#include <Nodes/NodeItem.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
Blackbox::Blackbox(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Blackbox"),
           tr("Blackbox data downloader"),
           Group,
           "dropbox")
{
    connect(Vehicles::instance(), &Vehicles::nodeNotify, this, &Blackbox::nodeNotify);

    f_import = new BlackboxFile(this);
}
//=============================================================================
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
//=============================================================================
