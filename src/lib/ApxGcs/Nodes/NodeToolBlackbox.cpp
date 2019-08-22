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
#include "NodeToolBlackbox.h"
#include "NodeItem.h"
#include "Nodes.h"
#include <App/AppRoot.h>
#include <ApxLog.h>

#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryRecorder.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>

#include <Xbus/XbusNodeBlackbox.h>
#include <common/Escaped.h>
//=============================================================================
NodeToolBlackbox::NodeToolBlackbox(Fact *parent, NodeItem *node, const QString &title)
    : NodeToolsGroup(parent, node, "blackbox", title, "", Group)
    , Escaped()
    , bb_read(0)
    , rec_size(0)
    , req_blk(0)
    , op(op_idle)
    , protocol(nullptr)
{
    setIcon("database");
}
//=============================================================================
Fact *NodeToolBlackbox::addCommand(QString name, QString title, QString descr, quint16 cmd)
{
    if (!name.contains("read", Qt::CaseInsensitive))
        return NodeToolsGroup::addCommand(name, title, descr, cmd);

    //bb_read command
    bb_read = cmd;

    f_read = new Fact(this, name, title, "", Group);
    f_read->setIcon("download");
    connect(f_read, &Fact::triggered, f_read, &Fact::requestDefaultMenu);

    f_callsign = new Fact(f_read, "callsign", tr("Callsign"), tr("Vehicle identity"), Text);
    if (!node->nodes->vehicle->isLocal() && node->nodes->vehicle->vehicleClass() != Vehicle::GCU) {
        f_callsign->setValue(node->nodes->vehicle->callsign());
    }
    //f_notes = new Fact(f_read, "notes", tr("Notes"), tr("Telemetry record notes"), Text);

    f_stats = new Fact(f_read, "stats", "", "", Const);
    connect(f_stats, &Fact::triggered, this, &NodeToolBlackbox::getStats);

    connect(this, &Fact::triggered, f_stats, &Fact::trigger);
    connect(f_read, &Fact::menuRequested, f_stats, &Fact::trigger);

    f_begin = new Fact(f_read, "begin", tr("Start block"), "", Int);
    f_begin->setMin(0);
    f_begin->setValue(0);

    f_end = new Fact(f_read, "end", tr("End block"), "", Int);
    f_end->setMin(0);

    //actions
    f_start = new FactAction(f_read, "start", tr("Start"), tr("Download data"), "download");
    connect(f_start, &FactAction::triggered, this, &NodeToolBlackbox::download);

    f_stop = new FactAction(f_read,
                            "stop",
                            tr("Stop"),
                            tr("Stop downloading"),
                            "stop",
                            FactAction::ActionStop);
    connect(f_stop, &FactAction::triggered, this, &NodeToolBlackbox::stop);

    connect(this, &Fact::progressChanged, this, &NodeToolBlackbox::updateActions);

    updateActions();

    connect(node, &NodeItem::serviceDataReceived, this, &NodeToolBlackbox::serviceDataReceived);
    connect(node, &NodeItem::requestTimeout, this, [this](quint16 cmd, QByteArray) {
        if (cmd > 0 && cmd == bb_read) {
            apxMsgW() << tr("Blackbox download timeout");
            stop();
        }
    });
    stop();
    return nullptr;
}
//=============================================================================
void NodeToolBlackbox::updateActions()
{
    f_start->setEnabled(progress() < 0 && op == op_idle && rec_size > 0);
    f_stop->setEnabled(op != op_idle);

    f_begin->setEnabled(rec_size);
    f_end->setEnabled(rec_size);

    //propagate progress
    f_read->setProgress(progress());
    f_stats->setProgress(progress());
    node->setProgress(progress());
}
//=============================================================================
void NodeToolBlackbox::request(Operation op, QByteArray data)
{
    if (progress() < 0)
        setProgress(0);
    this->op = op;
    node->requestUser(bb_read, data, 1000);
    updateActions();
}
void NodeToolBlackbox::request(quint32 blk)
{
    if (rec_size == 0 || blk >= rec_size) {
        stop();
        return;
    }
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
                           .arg(blk * (512.0 / 1024.0 / 1024.0), 0, 'f', 2)
                           .arg(sz * (512.0 / 1024.0 / 1024.0), 0, 'f', 2));
}
void NodeToolBlackbox::getStats()
{
    if (op == op_read)
        return;
    f_stats->setTitle(tr("Getting blackbox info"));
    request(op_stats, QByteArray());
}

void NodeToolBlackbox::download()
{
    req_begin = f_begin->value().toUInt();
    req_end = f_end->value().toUInt();
    if (op != op_idle || rec_size == 0 || req_begin >= req_end || protocol)
        return;
    apxMsg() << tr("Blackbox downloading").append("...");
    f_stats->setTitle(tr("Downloading").append("..."));

    //create tmp vehicle
    ProtocolVehicles::IdentData ident;
    ident.uid = node->sn();
    ident.vclass = Vehicle::TEMPORARY;
    QStringList st;
    QString s = f_callsign->text().trimmed();
    if (!s.isEmpty())
        st << s;
    st << "BLACKBOX";
    ident.callsign = st.join('-');

    protocol = new ProtocolVehicle(0, ident, nullptr);
    vehicle = Vehicles::instance()->createVehicle(protocol);
    vehicle->setParentFact(f_read);
    node->nodes->vehicle->setActive(false);

    request(req_begin);
}

void NodeToolBlackbox::stop()
{
    op = op_idle;
    setProgress(-1);
    f_stats->setTitle(rec_size ? tr("Statistics") : tr("Empty"));
    f_stats->setStatus(QString("%1 MB").arg(rec_size * (512.0 / 1024.0 / 1024.0), 0, 'f', 2));
    node->acknowledgeRequest(bb_read);
    if (protocol) {
        protocol->deleteLater();
        protocol = nullptr;
        vehicle->remove();
        Vehicles::instance()->selectVehicle(node->nodes->vehicle);
        node->nodes->vehicle->setActive(true);
    }
    esc_input.clear();
    esc_state = esc_cnt = esc_pos_save = 0;
}

void NodeToolBlackbox::serviceDataReceived(quint16 cmd, QByteArray data)
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
        rec_size = dHdr.rec_size * 16;

        f_begin->setMax(rec_size);
        f_end->setMax(rec_size);
        f_end->setValue(rec_size > 17 ? rec_size - 17 : rec_size);
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
        if (stream.tail() != (4 + 512))
            break;
        if (!protocol)
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
        processData(data.mid(stream.position()));
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
uint NodeToolBlackbox::esc_read(uint8_t *buf, uint sz)
{
    if (esc_input.isEmpty())
        return 0;
    uint cnt = static_cast<uint>(esc_input.size());
    if (cnt > sz)
        cnt = sz;
    memcpy(buf, esc_input.data(), cnt);
    esc_input.remove(0, static_cast<int>(cnt));
    return cnt;
}
void NodeToolBlackbox::escError(void)
{
    qWarning() << "stream read error";
    if (vehicle->f_telemetry->f_recorder->value().toBool())
        apxMsgW() << tr("Blackbox stream corrupted") << req_blk - 1;
}
//=============================================================================
void NodeToolBlackbox::processData(QByteArray data)
{
    esc_input.append(data);
    uint cnt;
    while ((cnt = readEscaped()) > 0) {
        QByteArray packet = QByteArray(reinterpret_cast<const char *>(esc_rx),
                                       static_cast<int>(cnt));

        vehicle->f_telemetry->f_recorder->setValue(true);
        protocol->unpack(packet);
    }
}
//=============================================================================
