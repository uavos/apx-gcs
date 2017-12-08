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
#include "Vehicles.h"
#include "Mandala.h"
#include "node.h"
#include <QtSql>
//=============================================================================
Nodes::Nodes(Vehicle *parent)
  : Fact(parent,"nodes","Nodes",tr("Vehicle components"),GroupItem,NoData),
    vehicle(parent),
    m_nodesCount(0)
{
  model()->setFlat(true);

  xml=new NodesXml(this);
  db=new NodesDB(this);

  f_request=new Fact(this,"request",tr("Request"),tr("Download from vehicle"),FactItem,ActionData);
  connect(f_request,&Fact::triggered,this,&Nodes::request);

  f_reload=new Fact(this,"reload",tr("Reload"),tr("Clear and download all"),FactItem,ActionData);
  connect(f_reload,&Fact::triggered,this,&Nodes::reload);
  connect(f_reload,&Fact::enabledChanged,this,&Nodes::actionsUpdated);

  f_upload=new Fact(this,"upload",tr("Upload"),tr("Upload modified values"),FactItem,ActionData);
  connect(f_upload,&Fact::triggered,this,&Nodes::upload);
  connect(f_upload,&Fact::enabledChanged,this,&Nodes::actionsUpdated);

  f_stop=new Fact(this,"stop",tr("Stop"),tr("Stop data requests"),FactItem,ActionData);
  connect(f_stop,&Fact::triggered,this,&Nodes::stop);
  connect(f_stop,&Fact::enabledChanged,this,&Nodes::actionsUpdated);

  f_list=new Fact(this,"list",tr("Nodes list"),"",SectionItem,NoData);

  connect(this,&Fact::modifiedChanged,this,&Nodes::updateActions);

  connect(this,&Nodes::nodesCountChanged,this,[=](){
    setStatus(nodesCount()>0?QString::number(nodesCount()):"");
    updateActions();
  });
  updateActions();

  if(!vehicle->isLocal())
    request();

  FactSystem::instance()->jsSync(this);
}
//=============================================================================
int Nodes::nodesCount() const
{
  return m_nodesCount;
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
    m_nodesCount++;
    emit nodesCountChanged();
  }
  return node;
}
//=============================================================================
void Nodes::updateProgress()
{
  int ncnt=0,v=0;
  foreach(NodeItem *node,snMap.values()){
    int np=node->progress();
    if(np || (!node->dictValid()) || (!node->dataValid()))ncnt++;
    v+=np;
  }
  if(ncnt){
    setProgress(v/ncnt);
  }else setProgress(0);
}
//=============================================================================
void Nodes::updateActions()
{
  bool busy=false;//model->requestManager.busy();
  bool upgrading=false;//model->isUpgrading();
  bool bModAll=modified();
  bool bEmpty=nodesCount()<=0;
  f_upload->setEnabled(bModAll && (!(busy)));
  f_stop->setEnabled(busy||upgrading);
  f_reload->setEnabled(!(upgrading||bEmpty));
}
//=============================================================================
//=============================================================================
void Nodes::request()
{
  vehicle->nmtManager->request(apc_search,QByteArray(),QByteArray(),0,true);
}
//=============================================================================
void Nodes::clear()
{
  if(snMap.isEmpty())return;
  snMap.clear();
  nGroups.clear();
  f_list->removeAll();
  setModified(false);
  f_list->setModified(false);
  FactSystem::instance()->jsSync(this);
}
//=============================================================================
void Nodes::reload()
{
  clear();
  request();
}
//=============================================================================
void Nodes::upload()
{
  foreach(NodeItem *node,snMap.values()){
    node->upload();
  }
}
//=============================================================================
void Nodes::stop()
{
}
//=============================================================================
void Nodes::nstat()
{
  foreach(NodeItem *node,snMap.values()){
    node->nstat();
  }
}
//=============================================================================
void Nodes::clearCache()
{
  foreach(NodeItem *node,snMap.values()){
    db->nodeDictClear(node);
  }
}
//=============================================================================
