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

#include <Fact/Fact.h>
#include <QtCore>
class DatalinkPort;
class Datalink;

class DatalinkPorts : public Fact
{
    Q_OBJECT
public:
    explicit DatalinkPorts(Datalink *datalink);

    Fact *f_list;

    DatalinkPort *f_add;

    void addPort(DatalinkPort *port);

    Q_INVOKABLE QStringList activeSerialPorts() const;

    Q_INVOKABLE void blockSerialPorts();
    Q_INVOKABLE void unblockSerialPorts();

    Datalink *datalink;

private:
    QList<DatalinkPort *> serialPorts() const;
    QList<DatalinkPort *> blockedPorts;

private slots:
    void updateStatus();

public slots:
    void addTriggered();

    void load();
    void save();
};
