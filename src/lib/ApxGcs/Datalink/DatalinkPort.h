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

class DatalinkPorts;
class Datalink;

class DatalinkPort : public Fact
{
    Q_OBJECT
    Q_ENUMS(PortType)

public:
    explicit DatalinkPort(DatalinkPorts *parent,
                          Datalink *datalink,
                          const DatalinkPort *port = nullptr);

    enum PortType { SERIAL, HTTP, UDP };
    Q_ENUM(PortType)

    Fact *f_enable;
    Fact *f_comment;
    Fact *f_type;
    Fact *f_url;
    Fact *f_baud;
    Fact *f_codec;

    Fact *f_routing;
    FactList f_rx;
    FactList f_tx;

    Fact *f_save;
    Fact *f_remove;

    DatalinkConnection *f_connection;

private:
    DatalinkPorts *ports;
    bool _new;
    bool _blockUpdateRoutingValue;
    bool _blockUpdateRoutingFacts;

    QUrl parseUrl() const;

private slots:
    void updateEnabled();

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
