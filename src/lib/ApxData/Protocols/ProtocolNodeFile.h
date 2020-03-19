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

#include "ProtocolBase.h"
#include "ProtocolNodeRequest.h"

class ProtocolNode;
class ProtocolNodeRequest;

class ProtocolNodeFile : public ProtocolBase
{
    Q_OBJECT
public:
    explicit ProtocolNodeFile(ProtocolNode *node, const QString &name);

    // called by nodes
    void downlink(xbus::node::file::op_e op, ProtocolStreamReader &stream);

    ProtocolNodeRequest *request(xbus::node::file::op_e op);

    static xbus::node::hash_t get_hash(const void *src, size_t sz, xbus::node::hash_t hash = 0);

private:
    ProtocolNode *node;

    xbus::node::file::info_s _info;
    xbus::node::file::op_e _op{xbus::node::file::idle};

    QPointer<ProtocolNodeRequest> _req{nullptr};
    QByteArray _op_data;
    xbus::node::file::offset_t _op_offset{0};
    xbus::node::file::size_t _op_size{0};
    xbus::node::file::size_t _op_tcnt{0};
    xbus::node::hash_t _op_hash{0};

    void reset();
    bool check_info(ProtocolStreamReader &stream);
    bool check_op(xbus::node::file::op_e op);
    void ack_req();
    bool resp_read(ProtocolStreamReader &stream);
    bool resp_write(ProtocolStreamReader &stream);

    void write_next();
    void read_next();

    void updateProgress();

public slots:
    void stop();
    void upload(QByteArray data, xbus::node::file::offset_t offset);
    void download();

signals:
    void finished();
    void error();
    void interrupted();

    void uploaded();
    void downloaded(const xbus::node::file::info_s &info, const QByteArray data);
};
