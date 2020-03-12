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

class Loader;
class Releases;
class FirmwareTools;

class Firmware : public Fact
{
    Q_OBJECT

public:
    explicit Firmware(Fact *parent);
    static Firmware *instance() { return _instance; }

    Fact *f_upgrade;
    Fact *f_stop;

    Releases *f_releases;

    Loader *f_loader;

    Fact *f_queue;
    Fact *f_available;

    FirmwareTools *f_tools;

    Q_INVOKABLE void requestUpgrade(const QString &nodeName,
                                    const QString &nodeDescr,
                                    const QString &sn,
                                    const QString &hw,
                                    const QString &ver,
                                    UpgradeType type);

    Q_INVOKABLE void requestInitialization(const QString &nodeName,
                                           const QString &hw,
                                           const QString &portName,
                                           Firmware::UpgradeType type);

private:
    static Firmware *_instance;

    QueueItem *queued(Fact *list, const QString &sn);

    int queueCnt;

private slots:
    void nodeUpdate(ProtocolNode *protocol);

    void updateStatus();
    void updateProgress();

    void next();
    void loaderFinished(bool success);

public slots:
    void stop();

signals:
    void upgradeStarted(QString sn, UpgradeType type);
    void upgradeFinished(QString sn, UpgradeType type);
};
