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
//=============================================================================
NodeTools::NodeTools(NodeItem *parent)
  : Fact(NULL,"tools",tr("Tools"),tr("Node tools"),GroupItem,NoData),
    node(parent)
{
  setIcon("wrench");
  setParent(parent);
  connect(parent,&Fact::removed,this,&Fact::removed);


  model()->setFlat(true);

  QString sect=tr("Backups");

  f_backups=new Fact(this,"backups",tr("Database"),tr("Restore parameters from backup"),GroupItem,NoData);
  f_backups->setIcon("database");
  f_backups->setSection(sect);

  Fact *f;
  f=new Fact(this,"recent",tr("Restore recent"),tr("Restore the most recent backup"),FactItem,NoData);
  f->setIcon("undo");
  f->setSection(sect);

  f_cmd=new Fact(this,"cmd",tr("Commands"),tr("Node hardware commands"),SectionItem,NoData);

  f_syscmd=new Fact(this,"syscmd",tr("Commands"),tr("System hardware commands"),SectionItem,NoData);

  sect=tr("System");
  f_update=new Fact(this,"firmware",tr("Firmware"),tr("Update node firmware"),GroupItem,NoData);
  f_update->setSection(sect);
  f_update->setIcon("chip");

  f=new Fact(this,"rebootall",tr("Reboot all"),tr("Vehicle system reset"),FactItem,NoData);
  f->setSection(sect);
  f->setIcon("reload");
}
//=============================================================================
void NodeTools::addCommand(uint cmd,const QString &name,const QString &descr,bool sys)
{
  Fact *fp=sys?f_syscmd:f_cmd;
  Fact *f=new Fact(fp,name,descr,"",FactItem,NoData);
  f->setIcon("eject");
  f->userData=cmd;
}
void NodeTools::clearCommands()
{
  f_cmd->removeAll();
  f_syscmd->removeAll();
}
//=============================================================================
