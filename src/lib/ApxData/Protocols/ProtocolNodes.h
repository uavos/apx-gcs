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

#include "ProtocolNode.h"

#include <QtCore>

class ProtocolVehicle;

class ProtocolNodes : public ProtocolBase
{
    Q_OBJECT

    Q_PROPERTY(bool valid READ valid WRITE setValid NOTIFY validChanged)

    Q_PROPERTY(bool upgrading READ upgrading WRITE setUpgrading NOTIFY upgradingChanged)

public:
    explicit ProtocolNodes(ProtocolVehicle *vehicle);

    friend class ProtocolNode;
    friend class ProtocolNodeRequest;

    ProtocolNodeRequest *request(mandala::uid_t uid, const QString &sn, size_t retry_cnt);

    ProtocolNode *getNode(QString sn, bool createNew = true);

    void acknowledgeRequest(const xbus::pid_s &pid, ProtocolStreamReader &stream);

    // called by vehicle
    void downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream);

    void syncLater(int time_ms, bool force_active);

    inline QList<ProtocolNode *> nodes() const { return _nodes.values(); }
    inline ProtocolVehicle *vehicle() const { return _vehicle; }

private:
    ProtocolVehicle *_vehicle;

    // nodes
    QHash<QString, ProtocolNode *> _nodes;

    // requests queue
    QList<ProtocolNodeRequest *> _queue;
    QElapsedTimer reqTime;
    QTimer finishedTimer;
    QTimer reqTimer;
    QTimer wdTimer;
    void schedule(ProtocolNodeRequest *request);
    xbus::pid_s _req_pid{};

    // sync management
    QTimer syncTimer;
    QElapsedTimer syncRequestTime;
    bool syncActive{false};
    int syncCount{0};

private slots:
    void next();

    void check_queue();
    void check_finished();

    void updateActive();
    void sendRequest(ProtocolNodeRequest *request);

    void syncTimeout();

    void updateValid();

    //export signals and slots
public slots:
    //nodes dict sync
    void requestSearch();

    void clear();
    void clear_requests();

    void requestStatus();

signals:
    void nodeNotify(ProtocolNode *protocol);
    void requestTimeout(ProtocolNodeRequest *request, ProtocolNode *node);
    void syncDone();

    //---------------------------------------
    // PROPERTIES
public:
    bool valid() const;
    void setValid(const bool &v);

    bool upgrading() const;
    void setUpgrading(const bool &v);

protected:
    bool m_valid{false};
    bool m_upgrading{false};

signals:
    void validChanged();
    void upgradingChanged();
};
