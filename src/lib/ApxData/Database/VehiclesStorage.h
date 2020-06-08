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
#pragma once

#include <QtCore>

#include <Protocols/ProtocolNode.h>

class ProtocolVehicle;

class VehiclesStorage : public QObject
{
    Q_OBJECT
public:
    explicit VehiclesStorage(ProtocolVehicle *vehicle, QObject *parent);

    void saveVehicleInfo();

    void loadNodeInfo(ProtocolNode *node);
    void saveNodeInfo(ProtocolNode *node);

    void saveNodeUser(ProtocolNode *node);

    void saveNodeDict(ProtocolNode *node, const ProtocolNode::Dict &dict);
    void saveNodeConfig(ProtocolNode *node, QVariantMap values);

    void saveConfiguration(bool force = false);
    void loadConfiguration(QString hash);

private:
    ProtocolVehicle *_vehicle;
private slots:
    void loadedConfiguration(QVariantMap configInfo, QList<QVariantMap> data);
};
