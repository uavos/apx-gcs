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

#include "ProtocolNode.h"

#include <QtCore>

class ProtocolVehicle;

class ProtocolNodes : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolNodes(ProtocolVehicle *vehicle);

    friend class ProtocolNode;
    friend class ProtocolNodeRequest;

    ProtocolNodeRequest *request(xbus::pid_t pid,
                                 const QString &sn,
                                 int timeout_ms,
                                 int retry_cnt = -1);

    ProtocolNode *getNode(QString sn, bool createNew = true);

    ProtocolNodeRequest *acknowledgeRequest(xbus::node::crc_t crc);
    ProtocolNodeRequest *acknowledgeRequest(ProtocolStreamReader &stream);
    ProtocolNodeRequest *extendRequest(xbus::node::crc_t crc, int timeout_ms);

    // called by vehicle
    void downlink(xbus::pid_t pid, ProtocolStreamReader &stream);

private:
    ProtocolVehicle *vehicle;

    // nodes
    QHash<QString, ProtocolNode *> _nodes;

    // requests queue
    QList<ProtocolNodeRequest *> _queue;
    QElapsedTimer reqTime;
    QTimer finishedTimer;
    QTimer reqTimer;
    QTimer wdTimer;
    void schedule(ProtocolNodeRequest *request);

private slots:
    void next();

    void check_queue();
    void check_finished();

    void updateActive();
    void sendRequest(ProtocolNodeRequest *request);

signals:
    void stopRequested();
    void queueEmpty();

    //export signals and slots
public slots:
    //nodes dict sync
    void requestSearch();
    void clear();
    void clear_requests();

signals:
    void nodeNotify(ProtocolNode *protocol);
    void requestTimeout(ProtocolNodeRequest *request, ProtocolNode *node);
};
