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

#include "QueueItem.h"

#include <App/AppGcs.h>
#include <Nodes/NodeItem.h>
#include <Nodes/Nodes.h>
#include <Vehicles/Vehicles.h>

class Releases;
class FirmwareTools;

class Firmware : public Fact
{
    Q_OBJECT

public:
    explicit Firmware(Fact *parent);
    static Firmware *instance() { return _instance; }

    Fact *f_start;
    Fact *f_stop;

    Fact *f_queue;
    Fact *f_available;

    FirmwareTools *f_tools;

    Q_INVOKABLE void requestUpgrade(QString uid, QString name, QString hw, QString type);

    Q_INVOKABLE void requestInitialize(
        QString name, QString hw, QString type, QString portName, bool continuous);

private:
    static Firmware *_instance;

    QMap<QString, QString> _nodesMap;

    QueueItem *queued(Fact *list, const QString &uid);

private slots:
    void vehicleRegistered(Vehicle *vehicle);
    void upgradeRequested(NodeItem *node, QString type); // captured by plugins

    void nodeNotify(NodeItem *node);

    void updateStatus();

    void next();
    void itemFinished(QueueItem *item, bool success);

    void stop();

signals:
    void upgradeStarted(QString uid, QString type);
    void upgradeFinished(QString uid, QString type);

    void nodesMapUpdated(QMap<QString, QString> nodes);
};
