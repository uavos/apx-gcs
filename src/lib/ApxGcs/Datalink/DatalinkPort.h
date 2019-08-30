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
#ifndef DatalinkPort_H
#define DatalinkPort_H
//=============================================================================
#include "DatalinkConnection.h"
#include <Fact/Fact.h>
#include <QtCore>
class DatalinkPorts;
class Datalink;
//=============================================================================
class DatalinkPort : public Fact
{
    Q_OBJECT
    Q_ENUMS(PortType)

public:
    explicit DatalinkPort(DatalinkPorts *parent,
                          Datalink *datalink,
                          const DatalinkPort *port = nullptr);

    enum PortType { SERIAL, TCP };
    Q_ENUM(PortType)

    Fact *f_enable;
    Fact *f_comment;
    Fact *f_type;
    Fact *f_url;
    Fact *f_baud;

    Fact *f_routing;
    QList<Fact *> f_rx;
    QList<Fact *> f_tx;

    Fact *f_save;
    Fact *f_remove;

    DatalinkConnection *f_connection;

private:
    DatalinkPorts *ports;
    bool _new;
    bool _blockUpdateRoutingValue;
    bool _blockUpdateRoutingFacts;

private slots:
    void updateStatus();

    void updateRoutingValue();
    void updateRoutingFacts();
    void updateRoutingStatus();
    void updateConnectionNetwork();

    void removeTriggered();

    void defaultUrl();
    void syncUrlEnum();
public slots:
    void defaults();
    void clear();
};
//=============================================================================
#endif
