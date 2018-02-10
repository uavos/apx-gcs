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
void NodeTools::addCommand(uint cmd,QString name,QString descr,bool sys)
{
  name=name.toLower();
  Fact *fp=sys?f_syscmd:f_cmd;
  if(descr.contains(':')){
    //grouping
    QString sgroup=descr.left(descr.indexOf(':')).trimmed();
    descr=descr.remove(0,descr.indexOf(':')+1).trimmed();
    Fact *fgroup=NULL;
    for(int i=0;i<fp->size();++i){
      if(fp->childFact(i)->title()!=sgroup)continue;
      fgroup=fp->childFact(i);
      break;
    }
    if(!fgroup){
      fgroup=new Fact(fp,sgroup,sgroup,"",GroupItem,NoData);
    }
    fp=fgroup;
  }
  Fact *f=new Fact(fp,name,descr,"",FactItem,NoData);
  if(name.contains("reboot")||name.contains("restart")) f->setIcon("reload");
  else if(name.contains("mute")) f->setIcon("volume-mute");
  else if(name.contains("erase")||name.contains("clear")) f->setIcon("close-circle");
  else if(name.contains("conf")) f->setIcon("alert-octagram");
  else if(name.startsWith("vm")) f->setIcon("code-braces");
  else if(name.startsWith("bb")) f->setIcon("database");
  else f->setIcon("asterisk");
  f->userData=cmd;
}
void NodeTools::clearCommands()
{
  f_cmd->removeAll();
  f_syscmd->removeAll();
}
//=============================================================================
