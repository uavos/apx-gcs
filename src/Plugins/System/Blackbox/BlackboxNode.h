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
#ifndef BlackboxNode_H
#define BlackboxNode_H
//=============================================================================
#include "BlackboxItem.h"
#include <Fact/Fact.h>
class NodeItem;
class Vehicle;
//=============================================================================
class BlackboxNode : public BlackboxItem
{
    Q_OBJECT

public:
    explicit BlackboxNode(Fact *parent, NodeItem *node);

    void addCommand(Fact *fact);

private:
    NodeItem *node;

    Fact *f_commands;

    Fact *f_begin;
    Fact *f_end;

    quint16 bb_read;
    quint32 rec_size;
    quint32 req_blk;
    quint32 req_begin;
    quint32 req_end;

    uint32_t block_size;

    enum Operation { op_idle, op_stats, op_read };
    Operation op;

    void request(Operation op, QByteArray data);
    void request(quint32 blk);

private slots:
    void updateTotals();

    void serviceDataReceived(quint16 cmd, QByteArray data);

public slots:
    void getStats() override;
    void download() override;
    void stop() override;
};
//=============================================================================
#endif
