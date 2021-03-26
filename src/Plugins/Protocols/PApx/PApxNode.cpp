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
#include "PApxNode.h"
#include "PApxNodeFile.h"
#include "PApxNodes.h"

#include <Mandala/Mandala.h>
#include <xbus/XbusNode.h>

PApxNode::PApxNode(PApxNodes *parent, QString uid)
    : PNode(parent, uid)
    , _req(this)
{}

PApxNode::~PApxNode()
{
    clear_requests();
}

void PApxNode::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    mandala::uid_t uid = pid.uid;

    if (uid == mandala::cmd::env::nmt::search::uid) {
        requestIdent();
        return;
    }

    //filter zero sized responses if any
    if (stream.available() == 0)
        return;

    // file ops - make them download data from any source
    if (uid == mandala::cmd::env::nmt::file::uid) {
        if (stream.available() <= sizeof(xbus::node::file::op_e))
            return;

        xbus::node::file::op_e op;
        stream >> op;

        if (op & xbus::node::file::reply_op_mask)
            trace()->block("re");

        op = static_cast<xbus::node::file::op_e>(op & ~xbus::node::file::reply_op_mask);
        trace()->block(QString::number(op));

        const char *s = stream.read_string(16);
        if (!s)
            return;

        trace()->block(QString(s));
        trace()->data(stream.payload());

        auto f = file(s);
        if (!f)
            return;

        f->process_downlink(op, stream);
    }

    // node messages
    if (uid == mandala::cmd::env::nmt::msg::uid) {
        if (stream.available() < (sizeof(xbus::node::msg::type_e) + 1))
            return;

        xbus::node::msg::type_e t;
        stream >> t;
        trace()->block(QString::number(t));

        const char *s = stream.read_string(stream.available());
        QString msg(QString(s).trimmed());
        trace()->block(msg);

        if (msg.isEmpty())
            return;

        msg.replace(":", ": ");
        msg = msg.simplified();

        emit messageReceived((msg_type_e) t, msg);
        return;
    }

    // check for requests responses
    if (pid.pri != xbus::pri_response) {
        // a request from another GCS?
        return;
    }

    // check for pending request's response
    auto req = _requests.value(uid);
    if (req) {
        // there's a request waiting for the packet's uid
        if (req->check_response(stream)) {
            delete_request(uid);
        }
    }
}

void PApxNode::schedule_request(PApxNodeRequest *req, mandala::uid_t uid)
{
    if (_requests.contains(uid)) {
        // the most recent for the uid is the only valid
        if (_requests.contains(mandala::cmd::env::nmt::ident::uid))
            return;
        qDebug() << "dup";
        delete_request(uid);
    }
    _requests.insert(uid, req);
    updateProgress();
    emit request_scheduled(req);
}

void PApxNode::delete_request(mandala::uid_t uid)
{
    auto req = _requests.value(uid);
    if (!req)
        return;
    emit request_finished(req);
    _requests.remove(uid);
    delete req;
    updateProgress();
}
void PApxNode::clear_requests()
{
    for (auto req : _requests) {
        emit request_finished(req);
        delete req;
    }
    _requests.clear();
    updateProgress();
}

void PApxNode::updateProgress()
{
    if (_requests.isEmpty()) {
        setProgress(-1);
        return;
    }
    if (progress() >= 0)
        return;
    setProgress(0);
}

void PApxNode::updateFiles(QStringList fnames)
{
    if (fnames == _files_map.keys()) {
        for (auto i : _files_map) {
            i->reset();
        }
        return;
    }

    for (auto i : _files_map) {
        i->deleteLater();
    }
    _files_map.clear();
    delete_request(mandala::cmd::env::nmt::file::uid);
    for (auto i : fnames) {
        _files_map.insert(i, new PApxNodeFile(this, i));
    }
}
