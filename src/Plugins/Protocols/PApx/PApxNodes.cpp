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
#include "PApxNodeRequest.h"

#include <Mandala/Mandala.h>
#include <XbusNode.h>

#include <App/AppLog.h>

#include <crc.h>

#define PAPX_REQ_DELAY_MS 0

#define PAPX_GCS_PART_SIZE 400             // bytes of data in packet
#define PAPX_GCS_SIZE_MAX (8 * 1024 * 1024) // bytes of compressed data
#define PAPX_GCS_IDLE_MS 1000              // requests idle time to send the downloaded data

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

    _gcsSend.setSingleShot(true);
    connect(&_gcsSend, &QTimer::timeout, this, &PApxNodes::sendGcsNodes);

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

bool PApxNodes::process_incoming_data(const xbus::pid_s &pid,
                                      PStreamReader &stream,
                                      bool is_remote_uplink)
{
    if (!mandala::cmd::env::nmt::match(pid.uid))
        return false;

    // if (is_remote_uplink) {
    //     trace()->data(stream.payload());
    //     return true;
    // }

    // if upgrading - forward all to local
    if (upgrading() && !_local) {
        auto local = findParent<PApx>()->local();
        trace()->block("LOCAL");
        trace()->tree();
        auto nodes = static_cast<PApxNodes *>(local->nodes());
        return nodes->process_incoming_data(pid, stream, is_remote_uplink);
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

    node->process_incoming_data(pid, stream);

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
    connect(node, &PNode::confReceived, this, [this]() {
        if (!_gcs_loading)
            _gcs_updated = true; // downloaded from hardware
    });

    emit node_available(node);
    return node;
}

void PApxNodes::requestSearch()
{
    _req.request(mandala::cmd::env::nmt::search::uid);
    _req.send();
}

// Nodes data exchange between GCS instances.
// GCS which has the nodes downloaded replies to the search request of another GCS
// and also sends the data when its own downloading from hardware is finished.
// The receiver uses the data for the nodes which are not downloaded yet.
// packet format: <cmd.env.aux.gcs> <uid> <data>
//  <uid> is <nmt.search>
//  <data> is <hash> <size> <offset> <part of compressed JSON [{info, dict, values}]>

void PApxNodes::process_remote_request(PStreamReader &stream)
{
    // request without payload from another GCS instance
    xbus::pid_s pid;
    pid.read(&stream);
    findParent<PApx>()->trace_pid(pid);

    if (pid.uid != mandala::cmd::env::nmt::search::uid || pid.pri != xbus::pri_request)
        return;

    // another GCS searches for nodes
    _gcs_requested = true;
    _gcsSend.start(0);
}

void PApxNodes::sendGcsNodes()
{
    if (_local || upgrading())
        return;

    const bool requested = _gcs_requested;
    const bool updated = _gcs_updated && !_request && _requests.isEmpty();
    if (!requested && !updated)
        return;
    _gcs_requested = false;
    if (updated)
        _gcs_updated = false;

    QJsonArray nodes;
    for (auto i : _nodes) {
        const auto node = i->gcsData();
        if (!node.isEmpty())
            nodes.append(node);
    }
    if (nodes.isEmpty())
        return;

    const auto data = qCompress(QJsonDocument(nodes).toJson(QJsonDocument::Compact));
    const uint32_t size = data.size();
    const uint32_t hash = apx::crc32(data.data(), size);

    qDebug() << "nodes to gcs:" << nodes.size() << size << "bytes";

    for (uint32_t offset = 0; offset < size; offset += PAPX_GCS_PART_SIZE) {
        _req.request(mandala::cmd::env::aux::gcs::uid, xbus::pri_response);
        _req << mandala::cmd::env::nmt::search::uid;
        findParent<PApx>()->trace_uid(mandala::cmd::env::nmt::search::uid);
        _req << hash;
        _req << size;
        _req << offset;
        trace()->block(QString("%1/%2").arg(offset).arg(size));
        _req.append(data.mid(offset, PAPX_GCS_PART_SIZE));
        _req.send();
    }
}

void PApxNodes::process_gcs_data(const xbus::pid_s &pid, PStreamReader &stream)
{
    if (_local)
        return;

    if (stream.available() < sizeof(mandala::uid_t))
        return;

    mandala::uid_t uid;
    stream >> uid;
    findParent<PApx>()->trace_uid(uid);

    if (uid != mandala::cmd::env::nmt::search::uid || pid.pri != xbus::pri_response) {
        trace()->data(stream.payload());
        return;
    }

    if (stream.available() <= sizeof(uint32_t) * 3)
        return;

    uint32_t hash, size, offset;
    stream >> hash;
    stream >> size;
    stream >> offset;
    trace()->block(QString("%1/%2").arg(offset).arg(size));

    if (offset == 0) {
        // start of data, the most recent sender wins
        if (size == 0 || size > PAPX_GCS_SIZE_MAX)
            return;
        _gcs_rx_hash = hash;
        _gcs_rx_size = size;
        _gcs_rx_data.clear();
    }
    if (!_gcs_rx_size || hash != _gcs_rx_hash || size != _gcs_rx_size
        || offset != static_cast<uint32_t>(_gcs_rx_data.size()))
        return; // parts of another sender or non-sequental

    _gcs_rx_data.append(stream.payload());
    if (static_cast<uint32_t>(_gcs_rx_data.size()) < size)
        return;

    // all data received
    const auto data = _gcs_rx_data;
    _gcs_rx_data.clear();
    _gcs_rx_size = 0;

    if (static_cast<uint32_t>(data.size()) != size || apx::crc32(data.data(), size) != hash) {
        qWarning() << "gcs nodes data error";
        return;
    }
    const auto nodes = QJsonDocument::fromJson(qUncompress(data)).array();
    if (nodes.isEmpty()) {
        qWarning() << "gcs nodes json error";
        return;
    }

    qDebug() << "nodes from gcs:" << nodes.size() << size << "bytes";

    // requests to hardware for the loaded nodes are not needed
    _gcs_loading = true;
    QStringList st;
    for (const auto &i : nodes) {
        const auto node = i.toObject();
        if (loadGcsNode(node))
            st.append(node.value("info").toObject().value("name").toString());
    }
    _gcs_loading = false;

    if (st.isEmpty())
        return;

    apxMsg() << tr("Nodes received from GCS").append(':') << st.join(',');
}

bool PApxNodes::loadGcsNode(QJsonObject node)
{
    if (upgrading())
        return false;

    const auto uid = node.value("info").toObject().value("uid").toString();
    const auto guid = QByteArray::fromHex(uid.toUtf8());
    if (guid.size() != sizeof(xbus::node::guid_t) || guid.toHex().toUpper() != uid.toUtf8()) {
        qWarning() << "gcs node uid" << uid;
        return false;
    }

    auto f = getNode(uid);
    if (!f)
        return false;

    // the node downloaded from hardware is not changed
    if (!f->gcsData().isEmpty())
        return false;

    return f->loadGcsData(node);
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
    _gcsSend.stop();
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
        _gcsSend.start(PAPX_GCS_IDLE_MS);
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
            apxMsgW() << tr("NMT request dropped").append(':') << _request->title();
        }

        // clear all node requests
        cancel_requests(_request->node());
        return;
    }

    _retry--;
    if (!_request->silent) {
        apxMsgW() << tr("NMT timeout").append(':') << _request->title()
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
        _gcsSend.start(PAPX_GCS_IDLE_MS);
        return;
    }

    if (!_request)
        request_next();
}
