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
#include "ProtocolService.h"
#include "ProtocolVehicle.h"

#include <App/AppLog.h>
#include <Xbus/XbusNode.h>
//=============================================================================
QStringList ProtocolService::upgradingNodes;
//=============================================================================
ProtocolService::ProtocolService(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle)
    , vehicle(vehicle)
    , activeCount(0)
    , m_active(false)
{
    connect(this, &ProtocolService::sendUplink, vehicle, &ProtocolVehicle::sendUplink);
    connect(this,
            &ProtocolService::sendServiceRequest,
            vehicle,
            &ProtocolVehicle::sendServiceRequest);
    connect(vehicle, &ProtocolVehicle::serviceData, this, &ProtocolService::serviceData);

    connect(this,
            &ProtocolService::next,
            this,
            &ProtocolService::doNextRequest,
            Qt::QueuedConnection);

    //timer.setSingleShot(true);
    timer.setInterval(100);
    connect(&timer, &QTimer::timeout, this, &ProtocolService::next);

    finishedTimer.setSingleShot(true);
    finishedTimer.setInterval(100);
    connect(&finishedTimer, &QTimer::timeout, this, &ProtocolService::checkFinished);
}
//=============================================================================
void ProtocolService::checkFinished()
{
    //if(active() && pool.isEmpty())setActive(false);
    foreach (ProtocolServiceNode *node, nodes.values()) {
        node->setProgress(-1);
    }
    if (!active()) {
        activeCount = 0;
    }
    emit finished();
}
//=============================================================================
ProtocolServiceRequest *ProtocolService::request(
    QString sn, quint16 cmd, QByteArray data, int timeout_ms, bool highprio, int retry)
{
    finishedTimer.stop();
    ProtocolServiceRequest *request = nullptr;
    for (int i = 0; i < pool.size(); ++i) {
        ProtocolServiceRequest *r = pool.at(i);
        if (!r->equals(sn, cmd, data))
            continue;
        request = r;
        r->timeout_ms = timeout_ms;
        //qDebug()<<"dup req"<<cmd<<data.toHex();
        break;
    }
    if (!request) {
        //qDebug()<<cmd;
        request = new ProtocolServiceRequest(this, sn, cmd, data, timeout_ms, highprio, retry);
        connect(request,
                &ProtocolServiceRequest::finished,
                this,
                &ProtocolService::requestFinished); //,Qt::QueuedConnection);
        connect(this,
                &ProtocolService::stopRequested,
                request,
                &ProtocolServiceRequest::deleteLater);
        int ins = pool.size();
        for (int i = 0; i < pool.size(); i++) {
            if (pool.at(i)->highprio)
                continue;
            if (pool.at(i)->sn == sn)
                ins = i + 1;
        }
        //pool.append(request);
        pool.insert(ins, request);
    }
    if (highprio) {
        pool.removeAll(request);
        pool.insert(0, request);
    }
    if (cmd == xbus::node::apc_loader)
        doNextRequestImmediately();
    else if (activeCount == 0 || (!timer.isActive()))
        emit next();
    return request;
}
//=============================================================================
ProtocolServiceRequest *ProtocolService::acknowledgeRequest(QString sn, quint16 cmd, QByteArray data)
{
    ProtocolServiceRequest *r = nullptr;
    for (int i = 0; i < pool.size(); ++i) {
        ProtocolServiceRequest *ri = pool.at(i);
        if (!ri->confirms(sn, cmd, data))
            continue;
        ri->finish(true);
        r = ri;
    }
    return r;
}
//=============================================================================
void ProtocolService::stop()
{
    //qDebug()<<active()<<pool.size();
    emit stopRequested();
    reset();
}
void ProtocolService::reset()
{
    timer.stop();
    qDeleteAll(pool);
    pool.clear();
    activeCount = 0;
    finishedTimer.start();
}
//=============================================================================
void ProtocolService::doNextRequest()
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
    doNextRequestImmediately();
}
void ProtocolService::doNextRequestImmediately()
{
    //find and trigger next request in pool
    for (int i = 0; i < pool.size(); ++i) {
        ProtocolServiceRequest *r = pool.at(i);
        if (r->active)
            continue;
        if (r->cmd == xbus::node::apc_loader) {
            r->trigger();
        } else if (upgradingNodes.isEmpty() && (active() || vehicle->squawk)) {
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
//=============================================================================
void ProtocolService::requestFinished(ProtocolServiceRequest *request)
{
    //qDebug()<<request->cmd<<request->data.toHex().toUpper();
    pool.removeAll(request);
    request->deleteLater();
    if (activeCount)
        activeCount--;
    emit next();
}
//=============================================================================
ProtocolServiceNode *ProtocolService::getNode(QString sn, bool createNew)
{
    if (sn.isEmpty() || sn.count('0') == sn.size())
        return nullptr;
    if (nodes.contains(sn))
        return nodes.value(sn);
    if (!createNew)
        return nullptr;
    ProtocolServiceNode *node = new ProtocolServiceNode(this, sn);
    connect(node, &QObject::destroyed, this, [=]() { nodes.remove(sn); });
    nodes.insert(sn, node);
    emit nodeFound(sn, node);
    return node;
}
//=============================================================================
void ProtocolService::clearNodes()
{
    if (!checkUpgrading())
        stop();
    qDeleteAll(nodes.values());
    nodes.clear();
}
void ProtocolService::removeNode(QString sn)
{
    if (!nodes.contains(sn))
        return;
    nodes.value(sn)->deleteLater();
    nodes.remove(sn);
}
//=============================================================================
//=============================================================================
void ProtocolService::serviceData(QString sn, quint16 cmd, QByteArray data)
{
    //qDebug()<<"service"<<sn<<cmd<<data.size();
    if (cmd == xbus::node::apc_loader) {
        if (data.isEmpty())
            return;
        emit loaderServiceData(sn, static_cast<quint8>(data.at(0)), data.mid(1));
        return;
    }
    reqTime.start();
    ProtocolServiceNode *node = getNode(sn);
    if (!node)
        return;
    node->serviceData(cmd, data);
}
//=============================================================================
void ProtocolService::requestNodes()
{
    if (checkUpgrading())
        return;
    request(QString(), xbus::node::apc_search, QByteArray(), 0, true);
}
//=============================================================================
void ProtocolService::rebootAll()
{
    if (checkUpgrading())
        return;
    setActive(true);
    request(QString(), xbus::node::apc_reboot, QByteArray(), 0, true);
}
//=============================================================================
//=============================================================================
void ProtocolService::nodeUpgrading(const QString &sn)
{
    if (upgradingNodes.contains(sn))
        return;
    upgradingNodes.append(sn);
}
void ProtocolService::nodeUpgradingFinished(const QString &sn)
{
    upgradingNodes.removeOne(sn);
}
bool ProtocolService::checkUpgrading()
{
    if (upgradingNodes.isEmpty())
        return false;
    //apxMsgW() << tr("Firmware upgrading in progress").append("...");
    return true;
}
//=============================================================================
//=============================================================================
bool ProtocolService::active() const
{
    return m_active;
}
void ProtocolService::setActive(bool v)
{
    if (m_active == v)
        return;
    m_active = v;
    emit activeChanged();
    qDebug() << v;
}
//=============================================================================
