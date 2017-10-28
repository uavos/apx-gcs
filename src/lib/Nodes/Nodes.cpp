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
#include "Nodes.h"
#include "NodeFact.h"
#include "Vehicle.h"
#include "Mandala.h"
#include "node.h"
//=============================================================================
Nodes::Nodes(Vehicle *parent)
  : Fact(parent,"nodes","Nodes",tr("Vehicle components"),GroupItem,NoData)
{
  setFlatModel(true);

  f_request=new Fact(this,"request",tr("Request"),tr("Download from vehicle"),FactItem,NoData);

  f_list=new Fact(this,"list",tr("Nodes list"),"",SectionItem,ConstData);
  bind(f_list);

}
//=============================================================================
bool Nodes::unpackService(const QByteArray &ba)
{
  if(ba.size()<(int)bus_packet_size_hdr_srv)return false;
  _bus_packet &packet=*(_bus_packet*)ba.data();
  if(packet.id!=idx_service)return false;
  QByteArray sn((const char*)packet.srv.sn,sizeof(_node_sn));
  uint data_cnt=ba.size()-bus_packet_size_hdr_srv;

  if(isBroadcast(sn))return true; //request?

  switch(packet.srv.cmd){
    case apc_search:
      nodeCheck(sn);
    return true;
    case apc_info: {
      //fill available nodes
      if(!data_cnt)break;  //request
      if(data_cnt<sizeof(_node_name))break;
      if(data_cnt<sizeof(_node_info)){
        memset(packet.srv.data+data_cnt,0,sizeof(_node_info)-data_cnt);
      }
      NodeFact *node=nodeCheck(sn);
      _node_info ninfo;
      memcpy(&ninfo,packet.srv.data,sizeof(_node_info));
      ninfo.name[sizeof(_node_name)-1]=0;
      node->setTitle((const char*)ninfo.name);
    }break;
  }
  //error

  return true;
}
//=============================================================================
bool Nodes::isBroadcast(const QByteArray &sn) const
{
  for(int i=0;i<sn.size();i++)
    if(((const uint8_t*)sn.data())[i]!=0)return false;
  return true;
}
//=============================================================================
NodeFact * Nodes::nodeCheck(const QByteArray &sn)
{
  NodeFact *node=snMap.value(sn);
  if(!node){
    node=new NodeFact(this,sn);
    snMap.insert(sn,node);
  }
  return node;
}
//=============================================================================
