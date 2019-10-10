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
#include "BlackboxNode.h"
#include "BlackboxReader.h"

#include <App/AppLog.h>
#include <App/AppRoot.h>

#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

#include <Nodes/NodeItem.h>
#include <Nodes/Nodes.h>

#include <Xbus/XbusNodeBlackbox.h>
//=============================================================================
BlackboxNode::BlackboxNode(Fact *parent, NodeItem *node)
    : BlackboxItem(parent, node->name(), "", "", Group, node->sn())
    , node(node)
    , bb_read(0)
    , rec_size(0)
    , req_blk(0)
    , block_size(512)
    , op(op_idle)
{
    bind(node);
    connect(node, &Fact::removed, this, &Fact::remove);

    if (!node->nodes->vehicle->isLocal() && node->nodes->vehicle->vehicleClass() != Vehicle::GCU) {
        f_callsign->setValue(node->nodes->vehicle->callsign());
    }

    connect(this, &Fact::progressChanged, node, [this]() { this->node->setProgress(progress()); });

    f_begin = new Fact(this, "begin", tr("Start block"), "", Int);
    f_begin->setMin(0);
    f_begin->setValue(0);
    connect(f_begin, &Fact::valueChanged, this, &BlackboxNode::updateTotals);

    f_end = new Fact(this, "end", tr("End block"), "", Int);
    f_end->setMin(0);
    connect(f_end, &Fact::valueChanged, this, &BlackboxNode::updateTotals);

    f_commands = new Fact(this, "commands", tr("Commands"), "", Action | IconOnly, "dots-horizontal");

    connect(node, &NodeItem::serviceDataReceived, this, &BlackboxNode::serviceDataReceived);
    connect(node, &NodeItem::requestTimeout, this, [this](quint16 cmd, QByteArray) {
        if (cmd > 0 && cmd == bb_read) {
            apxMsgW() << tr("Blackbox download timeout");
            stop();
        }
    });

    connect(this, &Fact::triggered, this, &BlackboxItem::getStats);

    updateActions();
}
//=============================================================================
void BlackboxNode::updateTotals()
{
    quint64 begin = f_begin->value().toUInt();
    quint64 end = f_end->value().toUInt();
    if (end > begin)
        totalSize = (end - begin) * block_size;
    else
        totalSize = 0;
    updateStats();
    updateActions();
}
//=============================================================================
void BlackboxNode::addCommand(Fact *fact)
{
    if (!fact->name().contains("read", Qt::CaseInsensitive)) {
        Fact *f = new Fact(f_commands, fact->name(), "", "");
        f->bind(fact);
        return;
    }
    bb_read = fact->userData.toUInt();
    connect(fact, &Fact::triggered, this, &Fact::trigger);
}
//=============================================================================
void BlackboxNode::request(Operation op, QByteArray data)
{
    if (progress() < 0)
        setProgress(0);
    this->op = op;
    node->requestUser(bb_read, data, 1000);
    updateActions();
}
void BlackboxNode::request(quint32 blk)
{
    req_blk = blk;
    QByteArray data;
    data.resize(sizeof(uint32_t));
    XbusStreamWriter stream(reinterpret_cast<uint8_t *>(data.data()));
    uint32_t a = blk;
    stream << a;
    request(op_read, data);
    blk -= req_begin;
    quint32 sz = req_end - req_begin;
    setProgress(static_cast<int>(blk * 100 / sz));
    f_stats->setStatus(QString("%1/%2 MB")
                           .arg(blk * (block_size / 1024.0 / 1024.0), 0, 'f', 2)
                           .arg(sz * (block_size / 1024.0 / 1024.0), 0, 'f', 2));
}
void BlackboxNode::getStats()
{
    if (node->offline())
        return;
    if (op == op_read)
        return;
    BlackboxItem::getStats();
    request(op_stats, QByteArray());
}

void BlackboxNode::download()
{
    req_begin = f_begin->value().toUInt();
    req_end = f_end->value().toUInt();
    if (op != op_idle || req_begin >= req_end || reader)
        return;

    BlackboxItem::download();

    //disable current vehicle
    node->nodes->vehicle->setActive(false);

    request(req_begin);
}

void BlackboxNode::stop()
{
    BlackboxItem::stop();
    op = op_idle;
    node->acknowledgeRequest(bb_read);
    Vehicles::instance()->selectVehicle(node->nodes->vehicle);
}

void BlackboxNode::serviceDataReceived(quint16 cmd, QByteArray data)
{
    if (op == op_idle)
        return;
    if (cmd != bb_read)
        return;
    if (data.size() < 3)
        return;
    uint16_t psize = static_cast<uint16_t>(data.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamReader stream(pdata, psize);

    uint8_t sub_cmd;
    stream >> sub_cmd;
    switch (sub_cmd) {
    default:
        break;
    case xbus::node::blackbox::bbr_hdr: {
        if (op != op_stats)
            return;
        xbus::node::blackbox::Header dHdr;
        if (stream.tail() != dHdr.psize())
            break;
        dHdr.read(&stream);

        rec_size = dHdr.rec_size;
        if (dHdr.block_size != 0xFFFF && dHdr.block_size != 0 && (dHdr.block_size & 0xFF) == 0)
            block_size = dHdr.block_size;

        f_end->setValue(rec_size);
        QStringList st;
        st << QString("on: %1 times").arg(dHdr.oncnt);
        st << QString("up: %1").arg(AppRoot::timeToString(dHdr.uptime, true));
        f_stats->setDescr(st.join(", "));
        node->acknowledgeRequest(bb_read);
        stop();
        return;
    }
    case xbus::node::blackbox::bbr_data: {
        if (op != op_read)
            return;
        if (stream.tail() != (4 + block_size))
            break;
        if (!reader)
            break;
        uint32_t b;
        stream >> b;
        if (req_blk != b) {
            qDebug("Duplicate block");
            return;
        }
        //block ok
        req_blk++;
        node->acknowledgeRequest(bb_read);
        reader->processData(data.mid(stream.position()));
        //continue
        if (req_blk >= req_end) {
            apxMsg() << tr("Blackbox download finished");
            stop();
            return;
        }
        request(req_blk);
        return;
    }
    } //switch
    //error
    apxMsgW() << tr("Unknown blackbox response") << cmd << data.size();
}
//=============================================================================
