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
#include "DatalinkClient.h"
#include "DatalinkClients.h"
#include "DatalinkSocket.h"
#include "crc.h"
//-----------------------------------------------------------------------------
//=============================================================================
DatalinkClient::DatalinkClient(DatalinkClients *parent, QTcpSocket *tcp)
  : DatalinkSocket(parent,tcp->peerAddress().toString(),tcp,true,parent->f_datalink->f_name->text()),
    f_datalink(parent->f_datalink)
{
  connect(parent->f_alloff,&FactAction::triggered,this,&DatalinkSocket::disconnectSocket);

  connect(this,&Fact::triggered,this,&DatalinkClient::disconnectSocket);

  connect(this,&DatalinkSocket::disconnected,this,&DatalinkClient::disconnected,Qt::QueuedConnection);

  connect(this,&DatalinkSocket::packetReceived,f_datalink,&Datalink::packetReceivedFromClient);
  connect(f_datalink,&Datalink::sendPacketToClients,this,&DatalinkSocket::sendPacket);

  connect(this,&DatalinkSocket::httpRequest,f_datalink,&Datalink::httpRequest);
}
//=============================================================================
void DatalinkClient::disconnected()
{
  //delete on disconnect
  if(parentItem())parentItem()->removeItem(this);
}
//=============================================================================
//=============================================================================
