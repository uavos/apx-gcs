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
{
    // store ident to parse dict
    connect(this, &PNode::identReceived, this, [this](QJsonValue json) {
        _ident = json.toObject();
    });
}

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
        // accept both pid requests and responses
        size_t spos = stream.pos();

        if (stream.available() <= sizeof(xbus::node::file::op_e))
            return;

        xbus::node::file::op_e op;
        stream >> op;

        if (op & xbus::node::file::reply_op_mask)
            trace()->block("re");
        trace()->block(QString::number(op & ~xbus::node::file::reply_op_mask));

        const char *s = stream.read_string(16);
        if (!s)
            return;

        trace()->block(QString(s));
        trace()->data(stream.payload());

        auto f = file(s);
        if (!f)
            return;

        f->process_downlink(op, stream);

        stream.reset(spos);
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

void PApxNode::schedule_request(PApxNodeRequest *req)
{
    mandala::uid_t uid = req->uid();
    if (_requests.contains(uid)) {
        // the most recent for the uid is the only valid
        if (uid == mandala::cmd::env::nmt::ident::uid)
            return;
        qDebug() << "dup";
        delete_request(uid);
    }
    _requests.insert(uid, req);
    updateProgress();
    emit request_scheduled(req);
}
void PApxNode::reschedule_request(PApxNodeRequest *req)
{
    mandala::uid_t uid = req->uid();
    if (!_requests.contains(uid)) {
        qDebug() << "not exists";
        delete_request(uid);
        return;
    }
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
        PApxNodeFile *f = new PApxNodeFile(this, i);
        _files_map.insert(i, f);

        if (i == "dict") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseDictData);
        } else if (i == "conf") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseConfData);
        } else if (i == "script") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseScriptData);
        }
    }
}

void PApxNode::parseDictData(const xbus::node::file::info_s &info, const QByteArray data)
{
    PStreamReader stream(data);

    bool err = true;
    QJsonArray fields;

    do {
        // check node hash
        xbus::node::hash_t hash = _ident.value("hash").toInt();
        if (info.hash != hash) {
            qWarning() << "node hash error:" << QString::number(info.hash, 16)
                       << QString::number(hash, 16);
            break;
        }

        QStringList names;
        QList<int> groups;
        QList<int> group_idx;

        while (stream.available() > 4) {
            QJsonObject field;

            xbus::node::conf::type_e type_id = static_cast<xbus::node::conf::type_e>(
                stream.read<uint8_t>());

            QString type = xbus::node::conf::type_to_str(type_id);

            field.insert("type", type);
            field.insert("array", stream.read<uint8_t>());

            uint8_t group = stream.read<uint8_t>();

            QStringList st;
            QString name, title;

            switch (type_id) {
            case xbus::node::conf::group:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "group" << field.insert(group << st;
                name = st.at(0);
                title = st.at(1);
                group_idx.append(fields.size());
                break;
            case xbus::node::conf::command:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                name = st.at(0);
                title = st.at(1);
                break;
            default:
                st = stream.read_strings(2, stream.available());
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "field" << field.insert(type << field.insert(array << field.insert(group << st;
                name = st.at(0);
                if (!st.at(1).isEmpty())
                    field.insert("units", st.at(1));
            }
            if (name.isEmpty())
                break;

            groups.append(group);
            names.append(name);

            if (title.isEmpty())
                title = name;
            field.insert("title", title);

            // guess path prepended with groups
            QStringList path;
            path.append(name);
            for (auto i = group; i > 0;) {
                i--;
                if (i < group_idx.size()) {
                    int gidx = group_idx.at(i);
                    path.prepend(names.at(gidx));
                    i = groups.at(gidx);
                    continue;
                }
                qWarning() << "missing group:" << type << field.value("array").toInt() << group
                           << st;
                path.clear(); //mark error
                break;
            }
            field.insert("name", path.join('.'));

            fields.append(field);

            //qDebug() << field << st << stream.available();

            if (stream.available() == 0) {
                err = false;
                break;
            }
        }

    } while (0);

    if (err) {
        qWarning() << "dict error" << data.toHex().toUpper();
        return;
    }
    qDebug() << "dict parsed";
    //printf("%s", QJsonDocument(fields).toJson().data());

    emit dictReceived(fields);
}

void PApxNode::parseConfData(const xbus::node::file::info_s &info, const QByteArray data) {}

void PApxNode::parseScriptData(const xbus::node::file::info_s &info, const QByteArray data) {}
