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
    vehicle(parent)
{
  model()->setFlat(true);

  f_request=new Fact(this,"request",tr("Request"),tr("Download from vehicle"),FactItem,ActionData);
  connect(f_request,&Fact::triggered,this,&Nodes::request);

  f_reload=new Fact(this,"reload",tr("Reload"),tr("Clear and download all"),FactItem,ActionData);
  connect(f_reload,&Fact::triggered,this,&Nodes::reload);

  f_list=new Fact(this,"list",tr("Nodes list"),"",SectionItem,NoData);
  connect(f_list,&Nodes::sizeChanged,this,[=](){
    setStatus(f_list->size()>0?QString::number(f_list->size()):"");
  });

  if(vehicle->f_vclass->value().toInt()!=Vehicle::LOCAL)
    request();

  FactSystem::instance()->jsSync(this);

  dbRegister();
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
void Nodes::updateProgress()
{
  int ncnt=0,v=0;
  foreach(NodeItem *node,snMap.values()){
    int np=node->progress();
    if(np || (!node->dataValid()))ncnt++;
    v+=np;
  }
  if(ncnt){
    setProgress(v/ncnt);
  }else setProgress(0);
}
//=============================================================================
//=============================================================================
void Nodes::request()
{
  vehicle->nmtManager->request(apc_search,QByteArray(),QByteArray(),0,true);
}
//=============================================================================
void Nodes::reload()
{
  if(!snMap.isEmpty()){
    snMap.clear();
    f_list->removeAll();
    setModified(false);
    f_list->setModified(false);
    FactSystem::instance()->jsSync(this);
  }
  request();
}
//=============================================================================
void Nodes::nstat()
{
  foreach(NodeItem *node,snMap.values()){
    node->nstat();
  }
}
//=============================================================================
//=============================================================================
void Nodes::dbRegister()
{
  QSqlQuery query(*FactSystem::db());
  while(FactSystem::db()->isOpen()){
    //table of actual state of all nodes ever seen
    query.prepare(
      "CREATE TABLE IF NOT EXISTS Nodes ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "sn TEXT NOT NULL UNIQUE, "
      "date INTEGER DEFAULT 0, "
      "name TEXT, "
      "version TEXT, "
      "hardware TEXT, "
      "hash TEXT, "
      "fcnt INTEGER"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE UNIQUE INDEX IF NOT EXISTS idx_Nodes_sn ON Nodes (sn);");
    if(!query.exec())break;
    //table of node fields (structure)
    query.prepare(
      "CREATE TABLE IF NOT EXISTS NodesDict ("
      "key INTEGER PRIMARY KEY NOT NULL, "
      "sn TEXT NOT NULL, "
      "hash TEXT NOT NULL, "
      "date INTEGER DEFAULT 0, "
      "id INTEGER, "
      "name TEXT, "
      "title TEXT, "
      "descr TEXT, "
      "ftype TEXT, "
      "array INTEGER, "
      "opts TEXT, "
      "sect TEXT"
      ")");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDict_sn ON NodesDict (sn);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDict_hash ON NodesDict (hash);");
    if(!query.exec())break;
    query.prepare("CREATE INDEX IF NOT EXISTS idx_NodesDict ON NodesDict (sn,hash);");
    if(!query.exec())break;

    FactSystem::db()->commit();
    return;
  }
  qWarning() << "Nodes SQL error:" << query.lastError().text();
  qWarning() << query.lastQuery();
}
//=============================================================================
