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

#include "ProtocolStream.h"
#include <Mandala/Mandala.h>
#include <xbus/XbusNode.h>

class ProtocolNodes;
class ProtocolNode;

class ProtocolNodeRequest : public QObject, public ProtocolStreamWriter
{
    Q_OBJECT
public:
    explicit ProtocolNodeRequest(ProtocolNodes *nodes,
                                 const QString &sn,
                                 const xbus::pid_s &pid,
                                 size_t retry_cnt,
                                 size_t timeout_ms = 0);

    bool equals(const ProtocolNodeRequest *other);
    bool equals(const xbus::pid_s &pid, ProtocolStreamReader &stream);
    bool lessThan(const ProtocolNodeRequest *other);

    void extend(size_t ms);
    void finish(bool acknowledged = false);

    void schedule();

    bool active{false};
    bool acknowledged{false};

private:
    ProtocolNodes *nodes;
    ProtocolNode *node;
    xbus::pid_s _pid;
    QString _sn;
    size_t retry_cnt;
    size_t timeout_ms;

    uint8_t packet_buf[xbus::size_packet_max];

    QTimer timer;

    size_t retry{0};

    size_t stream_pos_s;

private slots:
    void triggerTimeout();

public slots:
    void trigger();

signals:
    void finished(ProtocolNodeRequest *request);
    void retrying(size_t retry, size_t cnt);
    void timeout();
};
