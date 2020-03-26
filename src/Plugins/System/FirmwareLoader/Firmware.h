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

#include "QueueItem.h"
#include <Fact/Fact.h>

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

    Releases *f_releases;

    Fact *f_queue;
    Fact *f_available;

    FirmwareTools *f_tools;

    static ProtocolNodes *nodes_protocol();

    Q_INVOKABLE void requestUpgrade(ProtocolNode *protocol, QString type);
    Q_INVOKABLE void requestInitialize(const QString &type,
                                       const QString &name,
                                       const QString &hw,
                                       const QString &portName,
                                       bool continuous);

private:
    static Firmware *_instance;

    QueueItem *queued(Fact *list, const QString &sn);

private slots:
    void nodeNotify(ProtocolNode *protocol);

    void updateStatus();

    void next();
    void loaderFinished(QueueItem *item, bool success);

signals:
    void upgradeStarted(QString sn, QString type);
    void upgradeFinished(QString sn, QString type);
};
