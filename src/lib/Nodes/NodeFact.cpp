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
#include "NodeFact.h"
#include "Nodes.h"
#include <node.h>
//=============================================================================
NodeFact::NodeFact(Nodes *parent, const QByteArray &sn)
  : Fact(parent->f_list,"node#","",sn.toHex().toUpper(),GroupItem,NoData),
    sn(sn)
{
  setSection(parent->f_list->title());
}
//=============================================================================
bool NodeFact::unpackService(const QByteArray &ba)
{
  return true;
}
//=============================================================================
//=============================================================================
