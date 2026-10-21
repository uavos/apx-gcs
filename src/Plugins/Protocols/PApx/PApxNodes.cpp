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
#include <Datalink/Datalink.h>

#include <crc.h>

#include <QtEndian>

#define PAPX_REQ_DELAY_MS 0

#define PAPX_GCS_PART_SIZE 400             // bytes of data in packet
#define PAPX_GCS_SIZE_MAX (8 * 1024 * 1024) // bytes of compressed data
#define PAPX_GCS_DELAY_MS 100              // delay to merge consequent requests of the same node
#define PAPX_GCS_RX_MAX 64                 // incomplete node transfers to keep

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

    auto datalink = findParent<Datalink>();
    _f_gcsnodes = datalink ? datalink->f_gcsnodes : nullptr;

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

    if (is_remote_uplink && pid.pri == xbus::pri_request)
        gcsNodeRequested(node, pid, stream);

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

    emit node_available(node);
    return node;
}

void PApxNodes::requestSearch()
{
    _req.request(mandala::cmd::env::nmt::search::uid);
    _req.send();
}

// Nodes data exchange between GCS instances.
// GCS which has a node downloaded from hardware replies to the addressed requests
// of another GCS for this node (ident, dict/conf/script file read) with the node data.
// Every node is sent individually. The receiver uses the data for the nodes
// which are not downloaded yet, but only when the 'Nodes from GCS' datalink option is enabled.
// packet format: <cmd.env.aux.gcs> <uid> <guid> <data>
//  <uid> is <nmt.ident>
//  <guid> is the node uid
//  <data> is <hash> <size> <offset> <part of compressed JSON {info, dict, values}>

void PApxNodes::gcsNodeRequested(PApxNode *node, const xbus::pid_s &pid, PStreamReader &stream)
{
    // addressed request from another GCS instance
    if (_local || upgrading())
        return;

    bool requested = false;
    switch (pid.uid) {
    default:
        return;
    case mandala::cmd::env::nmt::ident::uid:
        requested = true;
        break;
    case mandala::cmd::env::nmt::file::uid: {
        // open to read dict, conf or script
        const auto pos_s = stream.pos();
        if (stream.available() > sizeof(xbus::node::file::op_e)) {
            xbus::node::file::op_e op;
            stream >> op;
            const char *s = stream.read_string(16);
            requested = s && (op == xbus::node::file::info || op == xbus::node::file::ropen)
                        && PApxNode::isConfFile(s);
        }
        stream.reset(pos_s);
        break;
    }
    }
    if (!requested)
        return;

    for (const auto &i : _gcs_tx) {
        if (i == node)
            return;
    }
    _gcs_tx.append(node);
    _gcsSend.start(PAPX_GCS_DELAY_MS);
}

void PApxNodes::sendGcsNodes()
{
    const auto nodes = _gcs_tx;
    _gcs_tx.clear();

    if (_local || upgrading())
        return;

    for (const auto &node : nodes) {
        if (node)
            sendGcsNode(node);
    }
}

void PApxNodes::sendGcsNode(PApxNode *node)
{
    const auto jso = node->gcsData();
    if (jso.isEmpty())
        return; // not downloaded yet, the other GCS downloads it from hardware

    xbus::node::guid_t guid;
    memset(guid, 0, sizeof(guid));
    const auto src = QByteArray::fromHex(node->uid().toUtf8());
    memcpy(guid, src.data(), qMin(static_cast<size_t>(src.size()), sizeof(guid)));

    const auto data = qCompress(QJsonDocument(jso).toJson(QJsonDocument::Compact));
    const uint32_t size = data.size();
    const uint32_t hash = apx::crc32(data.data(), size);

    qDebug() << node->title() << "node to gcs:" << size << "bytes";

    for (uint32_t offset = 0; offset < size; offset += PAPX_GCS_PART_SIZE) {
        _req.request(mandala::cmd::env::aux::gcs::uid, xbus::pri_response);
        _req << mandala::cmd::env::nmt::ident::uid;
        findParent<PApx>()->trace_uid(mandala::cmd::env::nmt::ident::uid);
        _req.write(guid, sizeof(guid));
        trace()->block(node->title().append(':'));
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

    if (uid != mandala::cmd::env::nmt::ident::uid || pid.pri != xbus::pri_response) {
        trace()->data(stream.payload());
        return;
    }

    if (!gcsNodesEnabled()) {
        // nodes are always downloaded from hardware
        trace()->block("off");
        return;
    }

    if (stream.available() <= sizeof(xbus::node::guid_t) + sizeof(uint32_t) * 3)
        return;

    QByteArray guid(sizeof(xbus::node::guid_t), '\0');
    stream.read(guid.data(), sizeof(xbus::node::guid_t));
    const QString node_uid(guid.toHex().toUpper());
    trace()->block("GUID");

    uint32_t hash, size, offset;
    stream >> hash;
    stream >> size;
    stream >> offset;
    trace()->block(QString("%1/%2").arg(offset).arg(size));

    if (offset == 0) {
        // start of the node data, the most recent sender wins
        if (size == 0 || size > PAPX_GCS_SIZE_MAX)
            return;
        if (!_gcs_rx.contains(node_uid) && _gcs_rx.size() >= PAPX_GCS_RX_MAX)
            _gcs_rx.clear(); // drop incomplete transfers
        _gcs_rx.insert(node_uid, {hash, size, {}});
    } else if (!_gcs_rx.contains(node_uid)) {
        return; // start of the data was missed
    }

    auto &rx = _gcs_rx[node_uid];
    if (hash != rx.hash || size != rx.size || offset != static_cast<uint32_t>(rx.data.size()))
        return; // parts of another sender or non-sequential

    rx.data.append(stream.payload());
    if (static_cast<uint32_t>(rx.data.size()) < size)
        return;

    // all data received
    const auto data = _gcs_rx.take(node_uid).data;

    if (static_cast<uint32_t>(data.size()) != size || apx::crc32(data.data(), size) != hash) {
        qWarning() << "gcs node data error" << node_uid;
        return;
    }

    // qCompress header: expected uncompressed size, big-endian
    const uint32_t raw_size = qFromBigEndian<uint32_t>(data.constData());
    if (raw_size > PAPX_GCS_SIZE_MAX) {
        qWarning() << "gcs node data size" << raw_size;
        return;
    }

    const auto node = QJsonDocument::fromJson(qUncompress(data)).object();
    if (node.isEmpty()) {
        qWarning() << "gcs node json error" << node_uid;
        return;
    }
    const auto info = node.value("info").toObject();
    if (info.value("uid").toString() != node_uid) {
        qWarning() << "gcs node uid mismatch" << node_uid << info.value("uid").toString();
        return;
    }

    qDebug() << "node from gcs:" << info.value("name").toString() << size << "bytes";

    // requests to hardware for the loaded node are not needed
    _gcs_loading = true;
    const bool loaded = loadGcsNode(node);
    _gcs_loading = false;

    if (!loaded)
        return;

    apxMsg() << tr("Node received from GCS").append(':') << info.value("name").toString();
}

bool PApxNodes::gcsNodesEnabled() const
{
    return _f_gcsnodes && _f_gcsnodes->value().toBool();
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
    if (_requests.isEmpty())
        return;
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
    if (_requests.isEmpty())
        return;

    if (!_request)
        request_next();
}
