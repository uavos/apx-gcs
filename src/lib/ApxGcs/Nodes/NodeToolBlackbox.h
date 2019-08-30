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
#ifndef NodeToolBlackbox_H
#define NodeToolBlackbox_H
//=============================================================================
#include "NodeToolsGroup.h"
#include <Protocols/ProtocolVehicle.h>
#include <common/Escaped.h>
class Vehicle;
//=============================================================================
class NodeToolBlackbox : public NodeToolsGroup, public Escaped
{
    Q_OBJECT

public:
    explicit NodeToolBlackbox(Fact *parent, NodeItem *node, const QString &title);

    Fact *addCommand(QString name, QString title, QString descr, quint16 cmd) override;

private:
    Fact *f_read;

    Fact *f_callsign;
    Fact *f_notes;
    Fact *f_stats;

    Fact *f_begin;
    Fact *f_end;

    Fact *f_start;
    Fact *f_stop;

    quint16 bb_read;
    quint32 rec_size;
    quint32 req_blk;
    quint32 req_begin;
    quint32 req_end;

    enum Operation { op_idle, op_stats, op_read };
    Operation op;

    void request(Operation op, QByteArray data);
    void request(quint32 blk);

    //esc reader
    QByteArray esc_input;
    uint esc_read(uint8_t *buf, uint sz) override;
    void escError(void) override;
    void processData(QByteArray data);

    //decoding
    ProtocolVehicle *protocol;
    Vehicle *vehicle;

private slots:
    void updateActions();

    void serviceDataReceived(quint16 cmd, QByteArray data);

public slots:
    void getStats();
    void download();
    void stop();
};
//=============================================================================
#endif
