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

#include <QtCore>

class ProtocolVehicle;

class VehiclesStorage : public QObject
{
    Q_OBJECT
public:
    explicit VehiclesStorage(ProtocolVehicle *vehicle);

    void saveVehicleInfo();

    /*void loadNodeInfo(ProtocolNode *node);
    void saveNodeInfo(ProtocolNode *node);

    void saveNodeUser(ProtocolNode *node);

    void saveNodeDict(ProtocolNode *node, const ProtocolNode::Dict &dict);
    void loadNodeDict(ProtocolNode *node);

    void saveNodeConfig(ProtocolNode *node);
    void loadNodeConfig(ProtocolNode *node, quint64 key = 0);
    */

    void saveConfiguration(bool force = false);
    void loadConfiguration(QString hash);

    static QString backupTitle(quint64 time, QString title);

private:
    ProtocolVehicle *_vehicle;

    size_t importConfiguration(QList<QVariantMap> data);

private slots:
    void loadedConfiguration(QVariantMap configInfo, QList<QVariantMap> data);
};
