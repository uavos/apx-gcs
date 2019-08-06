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
#ifndef DatalinkPorts_H
#define DatalinkPorts_H
//=============================================================================
#include <Fact/Fact.h>
#include <QtCore>
class DatalinkPort;
class Datalink;
//=============================================================================
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

private:
    Datalink *datalink;

    QList<DatalinkPort *> serialPorts() const;
    QList<DatalinkPort *> blockedPorts;

private slots:
    void updateStatus();

public slots:
    void addTriggered();

    void load();
    void save();
};
//=============================================================================
#endif
