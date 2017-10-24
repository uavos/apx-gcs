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
#include "AppSettings.h"
#include "Datalink.h"
#include "DatalinkHost.h"
#include "DatalinkHosts.h"
#include "tcp_ports.h"
//=============================================================================
DatalinkHost::DatalinkHost(DatalinkHosts *parent, QString title, QHostAddress host)
  : DatalinkSocket(parent->f_list,title,new QTcpSocket(),false,parent->f_datalink->f_name->text()),
   host(host),
   container(parent),
   bReconnect(false)
{
  setValue(host.toString());
  //connect(this,&Fact::childValueChanged,this,&DatalinkHost::updateStats);

  setSection(parent->f_list->title());

  updateStatsTimer.setSingleShot(true);
  connect(&updateStatsTimer,&QTimer::timeout,this,&DatalinkHost::updateStats);

  connect(this,&DatalinkSocket::disconnected,this,&DatalinkHost::disconnected);

  connect(this,&DatalinkSocket::triggered,this,&DatalinkHost::connectToServer);

  connect(this,&DatalinkSocket::statusChanged,parent,&DatalinkHosts::updateStats);

  connect(this,&DatalinkSocket::packetReceived,this,&DatalinkHost::updateTimeout);

  reconnectTimer.setSingleShot(true);
  connect(&reconnectTimer,&QTimer::timeout,this,&DatalinkHost::reconnect);

  updateStats();
  parent->updateStats();
}
//=============================================================================
void DatalinkHost::updateStats()
{
  QString s;
  if(time.isValid()){
    int t=time.elapsed()/1000;
    s=QString("%1 (%2)").arg(t>=60?tr("No service"):tr("Alive")).arg(t==0?tr("now"):t>=60?QString("%1 %2").arg(t/60).arg(tr("min")):QString("%1 %2").arg(t).arg(tr("sec")));
    updateStatsTimer.start(t>60?60000:5000);
  }
  setDescr(s);
}
//=============================================================================
void DatalinkHost::updateTimeout()
{
  time.start();
  updateStatsTimer.start(1000);
}
//=============================================================================
void DatalinkHost::disconnected()
{
  if(bReconnect){
    setStatus(QString("%1 %2").arg(tr("Retry")).arg(retry));
    reconnectTimer.start(1000+(retry>100?100:retry)*200);
  }else{
    setStatus(QString());
  }
}
//=============================================================================
void DatalinkHost::connectToServer()
{
  if(active()){
    disconnectAll();
    return;
  }
  bReconnect=true;
  retry=0;
  reconnect();
}
void DatalinkHost::reconnect()
{
  if(host.isEqual(container->f_datalink->f_clients->server->serverAddress()) ||
     (container->f_datalink->f_binded->value().toBool() && isLocalHost(host))
     ){
    qWarning("%s: %s",tr("Connection to local host not allowed").toUtf8().data(),title().toUtf8().data());
    disconnectAll();
    return;
  }
  retry++;
  socket->abort();
  socket->connectToHost(host,TCP_PORT_SERVER);
}
//=============================================================================
void DatalinkHost::disconnectAll()
{
  bReconnect=false;
  disconnectSocket();
}
//=============================================================================
bool DatalinkHost::active()
{
  return connected()||bReconnect;
}
//=============================================================================
