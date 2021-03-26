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
#include <xbus/XbusNode.h>

#include <App/AppLog.h>

PApxNodes::PApxNodes(PApxVehicle *parent)
    : PNodes(parent)
    , _req(parent)
{
    _reqTimer.setSingleShot(true);
    connect(&_reqTimer, &QTimer::timeout, this, &PApxNodes::request_timeout);
}

bool PApxNodes::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    if (!mandala::cmd::env::nmt::match(pid.uid))
        return false;

    while (pid.pri != xbus::pri_response) {
        if (pid.uid == mandala::cmd::env::nmt::msg::uid)
            break;
        if (pid.uid == mandala::cmd::env::nmt::search::uid)
            break;
        return true;
    }

    if (stream.available() < sizeof(xbus::node::guid_t))
        return true;

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

    emit node_available(node);
    return node;
}

void PApxNodes::requestSearch()
{
    _req.request(mandala::cmd::env::nmt::search::uid);
    _req.send();
}

void PApxNodes::request_scheduled(PApxNodeRequest *req)
{
    qDebug() << Mandala::meta(req->uid()).path;
    _requests.append(req);
    if (_request)
        return;
    request_next();
}
void PApxNodes::request_finished(PApxNodeRequest *req)
{
    qDebug() << Mandala::meta(req->uid()).path;
    _requests.removeOne(req);
    if (_request && _request != req)
        return;
    _request = nullptr;

    _reqTimer.stop();
    request_next();
}

void PApxNodes::request_next()
{
    if (_request) {
        qDebug() << "still pending";
        return;
    }

    if (_requests.isEmpty()) {
        qDebug() << "empty";
        return;
    }

    _request = _requests.first();
    _retry = PApxNodeRequest::retries;
    request_current();
}

void PApxNodes::request_current()
{
    qDebug() << Mandala::meta(_request->uid()).path;
    _request->make_request(_req);
    _req.send();
    _reqTimer.start(_request->timeout_ms() ? _request->timeout_ms() : 100);
}

void PApxNodes::request_timeout()
{
    if (!_request)
        return;

    if (!_request->timeout_ms()) {
        _request->discard();
    }

    if (!_retry) {
        apxMsgW() << tr("NMT request dropped").append(':') << _request->node()->title()
                  << Mandala::meta(_request->uid()).name;

        _request->discard();
        return;
    }

    _retry--;
    apxMsgW() << tr("NMT timeout").append(':') << _request->node()->title()
              << Mandala::meta(_request->uid()).name
              << QString("(%1/%2)")
                     .arg(PApxNodeRequest::retries - _retry)
                     .arg(PApxNodeRequest::retries);

    request_current();
}
