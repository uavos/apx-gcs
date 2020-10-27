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

    f_defaults = new Fact(fact,
                          "defaults",
                          tr("Defaults"),
                          tr("Restore default values"),
                          Fact::Action | Fact::IconOnly,
                          "lock-reset");
    connect(f_defaults, &Fact::triggered, fact, &Fact::restoreDefaults);
}

void NodeViewActions::updateRevert()
{
    f_revert->setEnabled(_fact->modified());
}
