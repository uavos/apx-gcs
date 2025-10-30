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

#include "PApxNodeRequest.h"

class PApxNode;

class PApxNodeFile : public PTreeBase
{
    // listens and collects file data from read/write requests

    Q_OBJECT
public:
    explicit PApxNodeFile(PApxNode *node, const QString &name);

    // called by nodes
    void process_incoming_data(xbus::node::file::op_e op, PStreamReader &stream);

    void reset();

    PApxNode *node() const { return _node; }

private:
    PApxNode *_node;

    xbus::node::file::info_s _info{};

    QByteArray _data;

    xbus::node::file::offset_t _offset{};
    xbus::node::file::size_t _size{};
    xbus::node::file::size_t _tcnt{};
    xbus::node::hash_t _hash{};

    xbus::node::file::op_e _open_op{};

    bool read(PStreamReader &stream);
    bool check_info(PStreamReader &stream);

    void updateProgress();

private slots:
    void requestInfo() { new PApxNodeRequestFile(_node, name(), xbus::node::file::info); }

signals:
    void finished();
    void error();
    void interrupted();

    void downloaded(PApxNode *_node, const xbus::node::file::info_s &info, const QByteArray data);
    void uploaded(PApxNode *_node, const xbus::node::file::info_s &info, const QByteArray data);
};
