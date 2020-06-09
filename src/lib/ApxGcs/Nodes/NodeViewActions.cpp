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
#include "NodeViewActions.h"
#include "Nodes.h"

NodeViewActions::NodeViewActions(Fact *fact, Nodes *nodes)
    : QObject(fact)
    , _fact(fact)
    , _nodes(nodes)
{
    Fact *f = nodes->f_upload->createAction(fact);
    f->setOption(Fact::ShowDisabled, false);
    f->setOption(Fact::IconOnly, false);

    f_revert = new Fact(fact,
                        "revert",
                        tr("Revert"),
                        tr("Undo changes"),
                        Fact::Action | Fact::IconOnly,
                        "undo");
    connect(f_revert, &Fact::triggered, fact, &Fact::restore);
    connect(fact, &Fact::modifiedChanged, this, &NodeViewActions::updateRevert);
    updateRevert();
}

void NodeViewActions::updateRevert()
{
    f_revert->setEnabled(_fact->modified());
}
