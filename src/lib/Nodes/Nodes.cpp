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
#include "NodeItem.h"
#include "NodeField.h"
#include "Vehicle.h"
#include "Mandala.h"
#include "node.h"
//=============================================================================
Nodes::Nodes(Vehicle *parent)
  : Fact(parent,"nodes","Nodes",tr("Vehicle components"),GroupItem,NoData),
    vehicle(parent)
{
  setFlatModel(true);

  f_request=new Fact(this,"request",tr("Request"),tr("Download from vehicle"),FactItem,NoData);
  connect(f_request,&Fact::triggered,this,&Nodes::search);

  f_list=new Fact(this,"list",tr("Nodes list"),"",SectionItem,ConstData);
  bind(f_list);

  if(vehicle->f_vclass->value().toInt()!=Vehicle::LOCAL)
    search();
}
//=============================================================================
bool Nodes::unpackService(const QByteArray &packet)
{
  if(packet.size()<(int)bus_packet_size_hdr_srv)return false;
  _bus_packet &bus_packet=*(_bus_packet*)packet.data();
  if(bus_packet.id!=idx_service)return false;
  QByteArray sn((const char*)bus_packet.srv.sn,sizeof(_node_sn));
  uint data_cnt=packet.size()-bus_packet_size_hdr_srv;
  //qDebug()<<"SRV"<<ba.toHex().toUpper();
  if(isBroadcast(sn))return true; //request?
  NodeItem *node=nodeCheck(sn);
  return node->unpackService(bus_packet.srv.cmd,QByteArray((const char*)bus_packet.srv.data,data_cnt));
}
//=============================================================================
bool Nodes::isBroadcast(const QByteArray &sn) const
{
  for(int i=0;i<sn.size();i++)
    if(((const uint8_t*)sn.data())[i]!=0)return false;
  return true;
}
//=============================================================================
NodeItem * Nodes::nodeCheck(const QByteArray &sn)
{
  NodeItem *node=snMap.value(sn);
  if(!node){
    node=new NodeItem(this,sn);
    snMap.insert(sn,node);
  }
  return node;
}
//=============================================================================
//=============================================================================
void Nodes::search()
{
  vehicle->nmtManager->request(apc_search,QByteArray(),QByteArray(),0,true);
}
//=============================================================================
