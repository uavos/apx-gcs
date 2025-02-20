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
#include "PApxNodes.h"

#include "PApxNode.h"

#include <Mandala/Mandala.h>
#include <XbusNode.h>

#include <App/AppLog.h>

#define PAPX_REQ_DELAY_MS 0

PApxNodes::PApxNodes(PApxUnit *parent)
    : PNodes(parent)
    , _req(parent)
    , _local(parent->uid().isEmpty())
{
    _reqTimeout.setSingleShot(true);
    connect(&_reqTimeout, &QTimer::timeout, this, &PApxNodes::request_timeout);

    _reqNext.setSingleShot(true);
    connect(&_reqNext, &QTimer::timeout, this, &PApxNodes::request_current);

    connect(root(), &PBase::cancelRequests, this, [this]() { cancel_requests(nullptr); });

    // inactive unit has delay for nodes downloading
    connect(parent, &Fact::activeChanged, this, &PApxNodes::updateActive);
    connect(this, &PNodes::upgradingChanged, this, &PApxNodes::updateActive);
    updateActive();
}
void PApxNodes::updateActive()
{
    bool v = parent()->active() || (upgrading() && _local);

    _reqNext.setInterval(v ? PAPX_REQ_DELAY_MS : 1000);

    if (_reqNext.isActive()) {
        _reqNext.stop();
        _reqNext.start();
    }
}

bool PApxNodes::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    if (!xbus::cmd::node::match(pid.uid))
        return false;

    // if upgrading - forward all to local
    if (upgrading() && !_local) {
        auto local = findParent<PApx>()->local();
        trace()->block("LOCAL");
        trace()->tree();
        auto nodes = static_cast<PApxNodes *>(local->nodes());
        return nodes->process_downlink(pid, stream);
    }

    if (stream.available() < sizeof(xbus::node::guid_t)) {
        if (pid.pri != xbus::pri_request)
            qDebug() << "missing guid" << stream.available();
        return true;
    }

    QByteArray uid_ba(sizeof(xbus::node::guid_t), '\0');
    stream.read(uid_ba.data(), sizeof(xbus::node::guid_t));
    QString uid(uid_ba.toHex().toUpper());

    // filter broadcast requests
    if (uid.isEmpty() || uid.count('0') == uid.size())
        return true;

    trace()->block("GUID");

    PApxNode *node = getNode(uid);
    if (!node)
        return true;

    trace()->block(node->title().append(':'));
    trace()->tree();

    node->process_downlink(pid, stream);

    if (pid.pri == xbus::pri_response)
        emit node_response(node);

    return true;
}

PApxNode *PApxNodes::getNode(QString uid, bool createNew)
{
    if (uid.isEmpty() || uid.count('0') == uid.size())
        return nullptr;

    PApxNode *node = _nodes.value(uid, nullptr);
    if (node)
        return node;

    if (!createNew)
        return nullptr;

    node = new PApxNode(this, uid);
    _nodes.insert(uid, node);
    connect(node, &Fact::removed, this, [this, node]() { _nodes.remove(_nodes.key(node)); });
    connect(node, &PApxNode::request_scheduled, this, &PApxNodes::request_scheduled);
    connect(node, &PApxNode::request_finished, this, &PApxNodes::request_finished);
    connect(node, &PApxNode::request_extended, this, &PApxNodes::request_extended);

    emit node_available(node);
    return node;
}

void PApxNodes::requestSearch()
{
    _req.request(xbus::cmd::node::search);
    _req.send();
}

void PApxNodes::request_scheduled(PApxNodeRequest *req)
{
    // qDebug() << req->title();
    if (_requests.contains(req)) {
        // rescheduled request
        if (_request != req) // not current
            return;
        _retry = PApxNodeRequest::retries;
        _reqTimeout.stop();
        _reqNext.start();
        return;
    }
    _requests.append(req);
    if (_request)
        return;
    request_next();
}
void PApxNodes::request_finished(PApxNodeRequest *req)
{
    // qDebug() << req->title();
    _requests.removeOne(req);
    if (_request && _request != req)
        return;
    _request = nullptr;

    _reqTimeout.stop();
    if (_requests.isEmpty()) {
        return;
    }
    request_next();
}
void PApxNodes::request_extended(PApxNodeRequest *req, size_t time_ms)
{
    if (_request != req)
        return;
    _reqTimeout.stop();
    _reqTimeout.start(time_ms);
}

void PApxNodes::request_next()
{
    if (_request) {
        qDebug() << "pending";
        return;
    }

    if (_requests.isEmpty()) {
        qDebug() << "empty";
        return;
    }

    _request = _requests.first();
    _retry = PApxNodeRequest::retries;
    _reqNext.start();
}

void PApxNodes::request_current()
{
    if (!_request)
        return;
    // qDebug() << _request->title();
    if (!_request->make_request(_req)) {
        // qDebug() << "discarded";
        _request->discard();
        return;
    }
    _req.send();
    _reqTimeout.stop();
    _reqTimeout.start(_request->timeout_ms() ? _request->timeout_ms() : 100);
}

void PApxNodes::request_timeout()
{
    if (!_request)
        return;

    if (!_request->timeout_ms()) {
        _request->discard();
        return;
    }

    if (!_retry) {
        if (!_request->silent) {
            apxMsgW() << tr("SYS request dropped").append(':') << _request->title();
        }

        // clear all node requests
        cancel_requests(_request->node());
        return;
    }

    _retry--;
    if (!_request->silent) {
        apxMsgW() << tr("SYS timeout").append(':') << _request->title()
                  << QString("(%1/%2)")
                         .arg(PApxNodeRequest::retries - _retry)
                         .arg(PApxNodeRequest::retries);
    }

    _reqNext.start();
}
void PApxNodes::cancel_requests(PApxNode *node)
{
    //qDebug() << node;
    if (_request && (!node || _request->node() == node)) {
        _reqTimeout.stop();
        _request = nullptr;
    }
    for (auto req : _requests) {
        if (node && req->node() != node)
            continue;
        _requests.removeOne(req);
        req->finished();
        delete req;
    }
    if (_requests.isEmpty()) {
        return;
    }

    if (!_request)
        request_next();
}
