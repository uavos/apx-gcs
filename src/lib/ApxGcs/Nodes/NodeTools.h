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
#ifndef NodeTools_H
#define NodeTools_H
//=============================================================================
#include "LookupNodeBackup.h"
#include "NodeToolsGroup.h"
class NodeItem;
//=============================================================================
class NodeTools : public NodeToolsGroup
{
    Q_OBJECT

public:
    explicit NodeTools(NodeItem *anode, FactBase::Flags flags = FactBase::Flags(Group));

    Fact *addCommand(QString name, QString title, QString descr, quint16 cmd) override;
    void clearCommands();

    NodeToolsGroup *f_cmd;
    NodeToolsGroup *f_syscmd;
    NodeToolsGroup *f_maintenance;

    LookupNodeBackup *f_backups;

    Fact *f_restore;

    Fact *f_rebootall;

    Fact *f_updates;
};
//=============================================================================
#endif
