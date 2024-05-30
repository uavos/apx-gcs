/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "DatalinkConnection.h"
#include <Fact/Fact.h>
#include <QtCore>
#include <QtNetwork>
class Datalink;
class DatalinkRemote;

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
    DatalinkRemote *findRemote(QUrl url);

public:
    int connectedCount() const;
    void setConnectedCount(int v);

protected:
    int m_connectedCount;

private:
    QUdpSocket *udpDiscover;
    Datalink *datalink;

private slots:
    void updateStatus();

    //UDP discover service
    void discover(void);
    void discoverRead(void);

    void connectTriggered();

signals:
    void connectedCountChanged();
};
