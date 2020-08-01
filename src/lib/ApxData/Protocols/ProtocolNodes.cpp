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

ProtocolNodes::ProtocolNodes(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle, "nodes")
    , _vehicle(vehicle)
{
    setTitle(tr("Nodes"));
    setDescr(tr("Vehicle components"));
    setIcon("puzzle");
    setDataType(Count);

    connect(vehicle, &Fact::enabledChanged, this, [this]() {
        setEnabled(this->vehicle()->enabled());
    });

    connect(this, &ProtocolNodes::nodeNotify, vehicle->vehicles, &ProtocolVehicles::nodeNotify);

    connect(this, &Fact::activeChanged, this, &ProtocolNodes::updateActive);

    reqTime.start();
    reqTimer.setSingleShot(true);
    connect(&reqTimer, &QTimer::timeout, this, &ProtocolNodes::next);

    finishedTimer.setSingleShot(true);
    finishedTimer.setInterval(100);
    connect(&finishedTimer, &QTimer::timeout, this, &ProtocolNodes::check_finished);

    wdTimer.setInterval(500);
    connect(&wdTimer, &QTimer::timeout, this, &ProtocolNodes::next);

    syncTimer.setSingleShot(true);
    connect(&syncTimer, &QTimer::timeout, this, &ProtocolNodes::syncTimeout);

    connect(this, &ProtocolNodes::upgradingChanged, this, [this]() {
        setValue(upgrading() ? tr("Upgrading") : QVariant());
        if (!upgrading())
            syncLater(5000, true);
    });

    connect(vehicle, &ProtocolVehicle::activeChanged, this, [this]() {
        if (!this->vehicle()->active())
            setActive(false);
    });

    connect(vehicle->vehicles, &ProtocolVehicles::stopNmtRequests, this, [this]() {
        setActive(false);
    });
}

void ProtocolNodes::updateActive()
{
    qDebug() << active();

    if (!active()) {
        setProgress(-1);
        wdTimer.stop();
        clear_requests();
        return;
    }

    wdTimer.start();

    if (progress() < 0)
        setProgress(0);

    if (reqTimer.isActive()) {
        reqTimer.start(0);
    }
}

void ProtocolNodes::schedule(ProtocolNodeRequest *request)
{
    check_queue();

    if (!enabled()) {
        request->deleteLater();
        return;
    }

    finishedTimer.stop();

    // find and abort existing
    foreach (auto const r, _queue) {
        if (!r->equals(request))
            continue;
        if (r->active) {
            request->deleteLater();
            return;
        }
        _queue.removeAll(r);
        r->deleteLater();
        break;
    }

    int ins = _queue.size();
    for (int i = 0; i < _queue.size(); i++) {
        if (_queue.at(i)->lessThan(request))
            continue;
        ins = i;
        break;
    }
    _queue.insert(ins, request);

    next();
}
void ProtocolNodes::next()
{
    check_queue();

    if (_queue.isEmpty())
        return;

    ProtocolNodeRequest *req = _queue.first();
    if (req->active)
        return;

    if (!active()) {
        // background sync
        if (vehicle()->squawk() == 0) {
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
void ProtocolNodes::check_queue()
{
    //qDebug() << _queue.size();
    if (_queue.isEmpty()) {
        setProgress(-1);
        reqTimer.stop();
        if (!finishedTimer.isActive())
            finishedTimer.start();
        return;
    }
    if (progress() < 0)
        setProgress(0);
    finishedTimer.stop();
}
void ProtocolNodes::check_finished()
{
    if (!_queue.isEmpty()) {
        next();
        return;
    }
    if (upgrading())
        return;
    int cnt = _nodes.size();
    if (cnt <= 0)
        return;

    do {
        if (!valid())
            break;
        // exclude reconf nodes
        for (auto i : _nodes) {
            if (i->ident().flags.bits.reconf)
                cnt--;
        }
        bool bCntChanged = syncCount != cnt;
        syncCount = cnt;
        if (bCntChanged) {
            qDebug() << "sync" << vehicle()->title()
                     << QString("%1 sec").arg(syncRequestTime.elapsed() / 1000.0, 0, 'f', 1);
            break;
        }

        // all valid and in sync
        qDebug() << "sync done";
        if (!(syncActive && syncTimer.isActive()))
            setActive(false);
        emit syncDone();

        // save config
        if (!vehicle()->isLocal() || vehicle()->active())
            vehicle()->storage->saveConfiguration();

        return;
    } while (0);

    //schedule re-sync
    syncLater(active() ? 100 : 1000, active());
}
void ProtocolNodes::syncLater(int time_ms, bool force_active)
{
    if (force_active)
        syncActive = true;
    if (syncTimer.isActive() && time_ms > syncTimer.interval())
        time_ms = syncTimer.interval();
    syncTimer.start(time_ms);
    qDebug() << time_ms << vehicle()->title();
}
void ProtocolNodes::syncTimeout()
{
    if (syncActive) {
        syncActive = false;
        setActive(true);
    }
    requestSearch();
}

ProtocolNodeRequest *ProtocolNodes::request(mandala::uid_t uid, const QString &sn, size_t retry_cnt)
{
    _req_pid.uid = uid;
    _req_pid.pri = xbus::pri_request;
    ProtocolNodeRequest *req = new ProtocolNodeRequest(this, sn, _req_pid, retry_cnt);
    _req_pid.seq++;
    connect(req, &QObject::destroyed, this, [this, req]() {
        _queue.removeAll(req);
        next();
    });
    connect(req, &ProtocolNodeRequest::finished, req, [this](ProtocolNodeRequest *r) {
        _queue.removeAll(r);
        r->deleteLater();
    });

    return req;
}

void ProtocolNodes::acknowledgeRequest(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    for (auto i : _queue) {
        if (i->equals(pid, stream)) {
            qDebug() << "ack" << i;
            i->finish(true);
            break;
        }
    }
}

void ProtocolNodes::clear_requests()
{
    //qDebug() << active() << _queue.size();

    int cnt = _queue.size();
    reqTimer.stop();

    for (auto i : _queue) {
        if (i)
            i->deleteLater();
    }
    _queue.clear();

    if (cnt > 0)
        finishedTimer.start();
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
    connect(node, &ProtocolNode::validChanged, this, &ProtocolNodes::updateValid);
    emit nodeNotify(node);
    return node;
}

void ProtocolNodes::clear()
{
    clear_requests();
    qDeleteAll(_nodes.values());
    _nodes.clear();
    syncCount = 0;
    syncTimer.stop();
    setValid(false);
}

void ProtocolNodes::downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    if (!enabled())
        return;

    while (pid.pri != xbus::pri_response) {
        if (pid.uid == mandala::cmd::env::nmt::msg::uid)
            break;
        if (pid.uid == mandala::cmd::env::nmt::search::uid)
            break;
        return;
    }

    if (stream.available() < sizeof(xbus::node::guid_t))
        return;

    QByteArray sn_ba(sizeof(xbus::node::guid_t), '\0');
    stream.read(sn_ba.data(), sizeof(xbus::node::guid_t));
    QString sn(sn_ba.toHex().toUpper());

    trace_downlink(ProtocolTraceItem::GUID, "GUID");

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

    if (vehicle()->isLocal() && !active()) {
        request->finish();
        return;
    }

    reqTime.start();
    vehicle()->send(request->toByteArray());
}

void ProtocolNodes::requestSearch()
{
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::search::uid, QString(), 0);
    req->schedule();
    req->finish();
    finishedTimer.start(2500);
}
void ProtocolNodes::requestStatus()
{
    setActive(true);
    for (auto i : _nodes)
        i->requestStatus();
}

//---------------------------------------
// PROPERTIES
//---------------------------------------

bool ProtocolNodes::valid() const
{
    return m_valid;
}
void ProtocolNodes::setValid(const bool &v)
{
    if (m_valid == v)
        return;
    m_valid = v;
    emit validChanged();
}
void ProtocolNodes::updateValid()
{
    if (_nodes.isEmpty()) {
        setValid(false);
        return;
    }

    for (auto const i : _nodes) {
        if (i->valid())
            continue;
        setValid(false);
        return;
    }
    setValid(true);
}
bool ProtocolNodes::upgrading() const
{
    return m_upgrading;
}
void ProtocolNodes::setUpgrading(const bool &v)
{
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    emit upgradingChanged();
}
