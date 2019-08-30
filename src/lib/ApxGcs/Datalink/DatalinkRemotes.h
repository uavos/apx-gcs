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
#ifndef DatalinkRemotes_H
#define DatalinkRemotes_H
//=============================================================================
#include "DatalinkConnection.h"
#include <Fact/Fact.h>
#include <QtCore>
#include <QtNetwork>
class Datalink;
class DatalinkRemote;
//=============================================================================
class DatalinkRemotes : public Fact
{
    Q_OBJECT

    Q_PROPERTY(int connectedCount READ connectedCount NOTIFY connectedCountChanged)

public:
    explicit DatalinkRemotes(Datalink *datalink);

    Fact *f_discover;

    Fact *f_add;
    Fact *f_url;
    Fact *f_connect;

    Fact *f_servers;

    Fact *f_alloff;

    DatalinkRemote *registerHost(QUrl url);
    DatalinkRemote *remoteByAddr(QHostAddress addr);

private:
    QUdpSocket *udpDiscover;
    Datalink *datalink;

private slots:
    void updateStatus();

    //UDP discover service
    void discover(void);
    void discoverRead(void);

    void connectTriggered();

    //-----------------------------------------
    //PROPERTIES
public:
    int connectedCount() const;
    void setConnectedCount(int v);

protected:
    int m_connectedCount;
signals:
    void connectedCountChanged();
};
//=============================================================================
#endif
