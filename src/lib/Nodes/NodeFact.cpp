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
#include "NodeField.h"
#include <node.h>
//=============================================================================
NodeFact::NodeFact(Nodes *parent, const QByteArray &sn)
  : NodeData(parent,sn),
    conf_uid(-1),
    m_valid(false)
{
  qmlRegisterUncreatableType<NodeFact>("GCS.Node", 1, 0, "Node", "Reference only");

  setSection(parent->f_list->title());
  setFlatModel(true);

  //setQmlMenu("nodes/NodeMenuItem.qml");

  f_version=new Fact(this,"version",tr("Version"),tr("Firmware version"),FactItem,ConstData);
  f_hardware=new Fact(this,"hardware",tr("Hardware"),tr("Hardware generation"),FactItem,ConstData);

  f_fields=new Fact(this,"fields",tr("Parameters"),"",SectionItem,NoData);

  connect(f_version,&Fact::valueChanged,this,&NodeFact::updateStats);
  connect(f_hardware,&Fact::valueChanged,this,&NodeFact::updateStats);

  //new NodeField(this);
}
//=============================================================================
void NodeFact::updateStats()
{
  setDescr(QString("%1 %2\t%3").arg(f_hardware->text()).arg(f_version->text()).arg(QString(sn.toHex().toUpper())));
}
//=============================================================================
bool NodeFact::unpackService(uint ncmd, const QByteArray &ba)
{
  switch(ncmd){
    case apc_search:
    return true;
    case apc_info: {
      //fill available nodes
      if((uint)ba.size()<sizeof(_node_name))break;
      _node_info ninfo;
      memset(&ninfo,0,sizeof(_node_info));
      if((uint)ba.size()>sizeof(_node_info)){
        memcpy(&ninfo,ba.data(),sizeof(_node_info));
      }else{
        memcpy(&ninfo,ba.data(),ba.size());
      }
      ninfo.name[sizeof(_node_name)-1]=0;
      setName((const char*)ninfo.name);
      setTitle((const char*)ninfo.name);
      f_version->setValue(QString((const char*)ninfo.version));
      f_hardware->setValue(QString((const char*)ninfo.hardware));
      setReconf(ninfo.flags.conf_reset);
      setFwSupport(ninfo.flags.loader_support);
      setFwUpdating(ninfo.flags.in_loader);
      setAddressing(ninfo.flags.addressing);
      setRebooting(ninfo.flags.reboot);
      setBusy(ninfo.flags.busy);
      setFailure(false);
    }return true;
    case apc_nstat: {
      if(ba.size()!=(sizeof(_node_name)+sizeof(_node_status)))break;
      _node_status nstatus;
      memcpy(&nstatus,ba.data()+sizeof(_node_name),sizeof(_node_status));
      setVbat(nstatus.power.VBAT);
      setIbat(nstatus.power.IBAT);
      setErrCnt(nstatus.err_cnt);
      setCanRxc(nstatus.can_rxc);
      setCanAdr(nstatus.can_adr);
      setCanErr(nstatus.can_err);
      setCpuLoad(nstatus.load);
    }return true;
    case apc_conf_inf: {
      if(ba.size()!=sizeof(_conf_inf))break;
      quint64 uid=0;
      memcpy(&uid,ba.data(),sizeof(_conf_inf));
      _conf_inf conf_inf;
      memcpy(&conf_inf,ba.data(),sizeof(_conf_inf));
      if(conf_inf.cnt!=allFields.size() || uid!=conf_uid){
        f_fields->clear();
        conf_uid=uid;
        for(quint16 id=0;id<conf_inf.cnt;id++){
          allFields.append(new NodeField(this,id));
        }
        //qDebug()<<"fields created"<<conf_inf.cnt;
      }
    }return true;
    case apc_conf_dsc:
    case apc_conf_read:
    case apc_conf_write:
    {
      if(ba.size()<1)break;
      NodeField *field=allFields.value((unsigned char)ba.at(0),NULL);
      if(!field)return true;
      if(field->unpackService(ncmd,ba.mid(1)))return true;
    }break;
  }
  //error

  return true;
}
//=============================================================================
void NodeFact::groupFields(void)
{
  foreach (FactTree *i, allFields) {
    NodeField *f=static_cast<NodeField*>(i);
    //grouping
    Fact *groupParent=f_fields;
    while(f->descr().contains(':')){
      QString group=f->descr().left(f->descr().indexOf(':'));
      f->setDescr(f->descr().remove(0,f->descr().indexOf(':')+1).trimmed());
      Fact *groupItem=static_cast<Fact*>(groupParent->child(group.toLower()));
      if(!groupItem)
        groupItem=new Fact(groupParent,group.toLower(),group,"",GroupItem,NoData);
      if(groupParent==f_fields)
        groupItem->setSection(groupParent->title());
      groupParent=groupItem;
      if(f->title().contains('_') && f->title().left(f->title().indexOf('_'))==group)
        f->setTitle(f->title().remove(0,f->title().indexOf('_')+1));
      f->parentItem()->removeItem(f,false);
      groupItem->addItem(f);
      f->setSection("");
    }
  }//foreach field
}
//=============================================================================
//=============================================================================
bool NodeFact::valid() const
{
  return m_valid;
}
void NodeFact::setValid(const bool &v)
{
  if(m_valid==v)return;
  m_valid=v;
  emit validChanged();
}
//=============================================================================
//=============================================================================
