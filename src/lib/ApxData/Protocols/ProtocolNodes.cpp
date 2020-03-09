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
    : ProtocolBase(vehicle)
    , vehicle(vehicle)
    , activeCount(0)
    , m_active(false)
{
    connect(this, &ProtocolNodes::next, this, &ProtocolNodes::doNextRequest, Qt::QueuedConnection);

    //timer.setSingleShot(true);
    timer.setInterval(100);
    connect(&timer, &QTimer::timeout, this, &ProtocolNodes::next);

    finishedTimer.setSingleShot(true);
    finishedTimer.setInterval(100);
    connect(&finishedTimer, &QTimer::timeout, this, &ProtocolNodes::checkFinished);
}

void ProtocolNodes::checkFinished()
{
    //if(active() && pool.isEmpty())setActive(false);
    foreach (ProtocolNode *node, nodes.values()) {
        node->setProgress(-1);
    }
    if (!active()) {
        activeCount = 0;
    }
    emit finished();
}

void ProtocolNodes::schedule(ProtocolNodeRequest *request)
{
    finishedTimer.stop();

    // find and abort existing
    foreach (auto const r, pool) {
        if (!r->equals(request))
            continue;
        remove(r);
    }

    connect(request, &ProtocolNodeRequest::finished, this, &ProtocolNodes::requestFinished);

    int ins = pool.size();
    for (int i = 0; i < pool.size(); i++) {
        if (pool.at(i)->lessThan(request))
            continue;
        ins = i;
        break;
    }
    pool.insert(ins, request);

    if (activeCount == 0 || (!timer.isActive()))
        emit next();
}

ProtocolNodeRequest *ProtocolNodes::acknowledgeRequest(xbus::node::crc_t crc)
{
    ProtocolNodeRequest *r = nullptr;
    for (auto i : pool) {
        if (i->acknowledge(crc))
            r = i;
    }
    return r;
}
ProtocolNodeRequest *ProtocolNodes::acknowledgeRequest(ProtocolStreamReader &stream)
{
    return acknowledgeRequest(ProtocolNodeRequest::get_crc(stream.buffer(), stream.pos()));
}

void ProtocolNodes::stop()
{
    //qDebug()<<active()<<pool.size();
    emit stopRequested();
    reset();
}
void ProtocolNodes::reset()
{
    timer.stop();
    qDeleteAll(pool);
    pool.clear();
    activeCount = 0;
    finishedTimer.start();
}

void ProtocolNodes::doNextRequest()
{
    if (pool.isEmpty()) {
        finishedTimer.start();
        return;
    }
    finishedTimer.stop();
    if (activeCount >= (active() ? 1 : 1)) {
        //qDebug()<<activeCount;
        if (!timer.isActive())
            timer.start();
        return;
    }
    if (!reqTime.isValid())
        reqTime.start();
    if ((!active()) && (reqTime.elapsed() < 5000)) {
        if (!timer.isActive())
            timer.start();
        return;
    }

    foreach (auto const r, pool) {
        if (r->active)
            continue;

        if (active() || vehicle->squawk) {
            activeCount++;
            r->trigger();
        } else {
            //don't sync in background for local vehicles
            r->finish(); //drop request
        }
        reqTime.start();
        timer.start(); //to continue trigger
        return;
    }
}

void ProtocolNodes::requestFinished(ProtocolNodeRequest *request)
{
    //qDebug()<<request->cmd<<request->data.toHex().toUpper();
    if (remove(request))
        emit next();
}
bool ProtocolNodes::remove(ProtocolNodeRequest *request)
{
    bool rv = pool.removeAll(request) > 0;
    request->deleteLater();
    if (rv && request->active && activeCount)
        activeCount--;
    return rv;
}

ProtocolNode *ProtocolNodes::getNode(QString sn, bool createNew)
{
    if (sn.isEmpty() || sn.count('0') == sn.size())
        return nullptr;
    if (nodes.contains(sn))
        return nodes.value(sn);
    if (!createNew)
        return nullptr;
    ProtocolNode *node = new ProtocolNode(this, sn);
    connect(node, &QObject::destroyed, this, [this, sn]() { nodes.remove(sn); });
    nodes.insert(sn, node);
    emit nodeFound(sn, node);
    return node;
}

void ProtocolNodes::clearNodes()
{
    stop();
    qDeleteAll(nodes.values());
    nodes.clear();
}
void ProtocolNodes::removeNode(QString sn)
{
    if (!nodes.contains(sn))
        return;
    nodes.value(sn)->deleteLater();
    nodes.remove(sn);
}

void ProtocolNodes::downlink(xbus::pid_t pid, ProtocolStreamReader &stream)
{
    if (stream.available() < sizeof(xbus::node::guid_t))
        return;

    QByteArray sn_ba(sizeof(xbus::node::guid_t), '\0');
    stream.read(sn_ba.data(), sizeof(xbus::node::guid_t));
    QString sn(sn_ba.toHex().toUpper());

    //qDebug() << "nmt" << sn;

    // filter broadcast requests
    if (sn.isEmpty() || sn.count('0') == sn.size())
        return;

    //qDebug() << "service" << sn << cmd << data.size();
    reqTime.start();
    ProtocolNode *node = getNode(sn);
    if (!node)
        return;

    node->downlink(pid, stream);
}

void ProtocolNodes::sendRequest(ProtocolNodeRequest *request)
{
    vehicle->send(request->toByteArray());
}

void ProtocolNodes::requestSearch()
{
    schedule(new ProtocolNodeRequest(this, QString(), mandala::cmd::env::nmt::search::meta.uid));
}

void ProtocolNodes::requestRebootAll()
{
    schedule(new ProtocolNodeRequest(this, QString(), mandala::cmd::env::nmt::reboot::meta.uid));
}

bool ProtocolNodes::active() const
{
    return m_active;
}
void ProtocolNodes::setActive(bool v)
{
    if (m_active == v)
        return;
    m_active = v;
    emit activeChanged();
    qDebug() << v;
}
