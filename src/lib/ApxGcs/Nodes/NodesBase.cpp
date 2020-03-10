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
#include "NodesBase.h"
#include "Nodes.h"

NodesBase::NodesBase(
    Fact *parent, const QString &name, const QString &title, const QString &descr, Flags flags)
    : Fact(parent, name, title, descr, flags)
{
    connect(this, &Fact::parentFactChanged, this, &NodesBase::addActions);
    addActions();
}

void NodesBase::addActions()
{
    if (!parentFact())
        return;
    if (!actions().isEmpty())
        return;

    disconnect(this, &Fact::parentFactChanged, this, &NodesBase::addActions);

    f_revert = new Fact(this, "revert", tr("Revert"), tr("Undo changes"), Action, "undo");
    connect(f_revert, &Fact::triggered, this, &Fact::restore);
    connect(this, &Fact::modifiedChanged, f_revert, [this]() { f_revert->setEnabled(modified()); });
    f_revert->setEnabled(modified());

    Nodes *nodes = findParent<Nodes *>();
    if (!nodes)
        return;
    Fact *f = new Fact(this, nodes->f_upload->name(), "", "", Action);
    f->bind(nodes->f_upload);
}
