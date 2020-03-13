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
#include "ProtocolNodes.h"
#include "ProtocolVehicle.h"

#include <App/AppLog.h>

#include <Mandala/Mandala.h>
#include <Xbus/XbusNode.h>

#include <crc/crc.h>

ProtocolNodes::ProtocolNodes(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle, "nodes")
    , vehicle(vehicle)
{
    setIcon("puzzle");
    setDataType(Count);

    connect(vehicle, &Fact::enabledChanged, this, [this]() {
        setEnabled(this->vehicle->enabled());
    });

    connect(this, &Fact::activeChanged, this, &ProtocolNodes::updateActive);

    reqTime.start();
    reqTimer.setSingleShot(true);
    connect(&reqTimer, &QTimer::timeout, this, &ProtocolNodes::next);

    finishedTimer.setSingleShot(true);
    finishedTimer.setInterval(100);
    connect(&finishedTimer, &QTimer::timeout, this, &ProtocolNodes::queueEmpty);
}

void ProtocolNodes::updateActive()
{
    qDebug() << active();

    if (!active()) {
        stop();
        return;
    }
    if (progress() < 0)
        setProgress(0);

    if (reqTimer.isActive()) {
        reqTimer.start(0);
    }
}

void ProtocolNodes::schedule(ProtocolNodeRequest *request)
{
    if (!enabled()) {
        request->deleteLater();
        return;
    }

    finishedTimer.stop();

    // find and abort existing
    foreach (auto const r, _queue) {
        if (!r->equals(request))
            continue;
        remove(r);
    }

    int ins = _queue.size();
    for (int i = 0; i < _queue.size(); i++) {
        if (_queue.at(i)->lessThan(request))
            continue;
        ins = i;
        break;
    }
    _queue.insert(ins, request);
    connect(request, &ProtocolNodeRequest::finished, this, &ProtocolNodes::requestFinished);

    if (_queue.size() == 1)
        next();
}
void ProtocolNodes::next()
{
    if (_queue.isEmpty()) {
        reqTimer.stop();
        finishedTimer.start();
        return;
    }
    finishedTimer.stop();

    ProtocolNodeRequest *req = _queue.first();
    if (req->active)
        return;

    if (!active()) {
        // background sync
        if (vehicle->squawk() == 0) {
            req->finish();
            return;
        }
        qint64 t = reqTime.elapsed();
        if (t < 3120) {
            reqTimer.start(3120 - static_cast<int>(t));
            return;
        }
    }
    req->trigger();
}

ProtocolNodeRequest *ProtocolNodes::request(xbus::pid_t pid,
                                            const QString &sn,
                                            int timeout_ms,
                                            int retry_cnt)
{
    return new ProtocolNodeRequest(this, sn, pid, timeout_ms, retry_cnt);
}

ProtocolNodeRequest *ProtocolNodes::acknowledgeRequest(xbus::node::crc_t crc)
{
    ProtocolNodeRequest *r = nullptr;
    for (auto i : _queue) {
        if (i->equals(crc)) {
            i->acknowledge();
            r = i;
        }
    }
    return r;
}
ProtocolNodeRequest *ProtocolNodes::acknowledgeRequest(ProtocolStreamReader &stream)
{
    return acknowledgeRequest(ProtocolNodeRequest::get_crc(stream.buffer(), stream.pos()));
}
ProtocolNodeRequest *ProtocolNodes::extendRequest(xbus::node::crc_t crc, int timeout_ms)
{
    ProtocolNodeRequest *r = nullptr;
    for (auto i : _queue) {
        if (i->equals(crc)) {
            i->extend(timeout_ms);
            r = i;
        }
    }
    return r;
}

void ProtocolNodes::stop()
{
    //qDebug() << active() << _queue.size();
    emit stopRequested();
    int cnt = _queue.size();
    reqTimer.stop();
    qDeleteAll(_queue);
    _queue.clear();
    setProgress(-1);
    if (cnt > 0)
        finishedTimer.start();
}

void ProtocolNodes::requestFinished(ProtocolNodeRequest *request)
{
    //qDebug() << request->dump();
    if (remove(request))
        next();
}
bool ProtocolNodes::remove(ProtocolNodeRequest *request)
{
    //qDebug() << request->dump();
    request->deleteLater();
    return _queue.removeAll(request) > 0;
}

ProtocolNode *ProtocolNodes::getNode(QString sn, bool createNew)
{
    if (sn.isEmpty() || sn.count('0') == sn.size())
        return nullptr;
    ProtocolNode *node = _nodes.value(sn, nullptr);
    if (node)
        return node;
    if (!createNew)
        return nullptr;
    node = new ProtocolNode(this, sn);
    _nodes.insert(sn, node);
    emit nodeUpdate(node);
    return node;
}

void ProtocolNodes::clear()
{
    if (active()) {
        apxMsgW() << tr("Operation in progress");
        return;
    }
    stop();
    qDeleteAll(_nodes.values());
    _nodes.clear();
}

void ProtocolNodes::downlink(xbus::pid_t pid, ProtocolStreamReader &stream)
{
    if (!enabled())
        return;

    if (stream.available() < sizeof(xbus::node::guid_t))
        return;

    QByteArray sn_ba(sizeof(xbus::node::guid_t), '\0');
    stream.read(sn_ba.data(), sizeof(xbus::node::guid_t));
    QString sn(sn_ba.toHex().toUpper());

    //qDebug() << QString::number(pid, 16) << sn;
    //qDebug() << "nmt" << sn;

    // filter broadcast requests
    if (sn.isEmpty() || sn.count('0') == sn.size())
        return;

    //qDebug() << "service" << sn << cmd << data.size();
    ProtocolNode *node = getNode(sn);
    if (!node)
        return;

    node->downlink(pid, stream);
}

void ProtocolNodes::sendRequest(ProtocolNodeRequest *request)
{
    if (!enabled())
        return;
    reqTime.start();
    vehicle->send(request->toByteArray());
}

void ProtocolNodes::requestSearch()
{
    request(mandala::cmd::env::nmt::search::uid, QString(), 0)->schedule();
}
