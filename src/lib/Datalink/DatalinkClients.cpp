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
#include "Datalink.h"
#include "DatalinkClients.h"
#include "DatalinkClient.h"
#include "tcp_ports.h"
//=============================================================================
DatalinkClients::DatalinkClients(Datalink *parent)
  : Fact(parent,"clients",tr("Connected clients"),tr("Remote clients connections"),GroupItem,ConstData)
{
  setFlatModel(true);

  f_datalink=parent;

  f_alloff=new Fact(this,"alloff",tr("Disconnect all"),tr("Drop all client connections"),FactItem,NoData);

  f_list=new Fact(this,"list",tr("Clients list"),tr("Active connections"),SectionItem,ConstData);
  bindValue(f_list);
  connect(f_list,&Fact::structChanged,this,&DatalinkClients::updateStats);

  server=new QTcpServer(this);
  connect(server,&QTcpServer::newConnection,this,&DatalinkClients::newConnection);

  connect(f_datalink->f_active,&Fact::valueChanged,this,&DatalinkClients::serverActiveChanged);

  serverActiveChanged();
}
//=============================================================================
void DatalinkClients::updateStats()
{
  f_alloff->setEnabled(f_list->size()>0);
}
//=============================================================================
void DatalinkClients::serverActiveChanged()
{
  bool active=f_datalink->f_active->value().toBool();
  if(!active){
    f_alloff->trigger();
    server->close();
    f_datalink->f_binded->setValue(false);
    qDebug("%s",tr("Datalink server disabled").toUtf8().data());
    return;
  }
  //activate server
  qDebug("%s",tr("Datalink server enabled").toUtf8().data());
  retryBind=0;
  tryBindServer();
}
//=============================================================================
//=============================================================================
void DatalinkClients::tryBindServer()
{
  if(!f_datalink->f_active->value().toBool())return;
  if(!server->listen(QHostAddress::Any,TCP_PORT_SERVER)) {
    f_datalink->f_binded->setValue(false);
    //server port is busy by another local GCU
    if(++retryBind<=1){
      qWarning("%s: %s",tr("Unable to start server").toUtf8().data(),server->errorString().toUtf8().data());
    }
    uint to=1000+(retryBind/10)*1000;
    QTimer::singleShot(to>10000?10000:to,this,SLOT(tryBindServer()));
    return;
  }
  f_datalink->f_binded->setValue(true);
  if(retryBind)qDebug("%s",tr("Server binded").toUtf8().data());
  retryBind=0;
}
//=============================================================================
void DatalinkClients::newConnection()
{
  while(server->hasPendingConnections()){
    QTcpSocket *socket=server->nextPendingConnection();
    if(!f_datalink->f_active->value().toBool()){
      socket->disconnectFromHost();
      continue;
    }
    new DatalinkClient(this,socket);
  }
}
//=============================================================================


