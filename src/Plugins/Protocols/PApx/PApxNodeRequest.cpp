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
#include "PApxNodeRequest.h"
#include "PApxNode.h"
#include "PApxNodes.h"

#include <App/App.h>
#include <xbus/XbusNode.h>

PApxNodeRequest::PApxNodeRequest(PApxNode *node, mandala::uid_t uid, uint timeout_ms)
    : _node(node)
    , _uid(uid)
    , _timeout_ms(timeout_ms)
{
    node->schedule_request(this, uid);
}

PTrace *PApxNodeRequest::trace() const
{
    return _node->trace();
}
void PApxNodeRequest::discard()
{
    _node->delete_request(uid());
}

void PApxNodeRequest::make_request(PApxRequest &req)
{
    req.request(_uid);
    QByteArray src(QByteArray::fromHex(_node->uid().toUtf8()));
    size_t sz = static_cast<size_t>(src.size());
    if (sz > sizeof(xbus::node::guid_t)) {
        sz = sizeof(xbus::node::guid_t);
        qWarning() << "guid size:" << sz;
    }
    xbus::node::guid_t guid;
    memset(guid, 0, sizeof(guid));
    memcpy(guid, src.data(), sz);
    req.write(guid, sizeof(guid));
    _node->trace()->block(_node->title().append(':'));
    _node->trace()->tree();

    request(req);
}

bool PApxNodeRequestIdent::response(PStreamReader &stream)
{
    //trace()->data(stream.payload());
    if (stream.available() <= (xbus::node::ident::ident_s::psize() + 3 * 2)) {
        qWarning() << "size" << stream.available() << xbus::node::ident::ident_s::psize();
        return false;
    }
    xbus::node::ident::ident_s ident;
    ident.read(&stream);

    trace()->block("IDENT");

    QStringList st = stream.read_strings(3);
    if (st.isEmpty()) {
        qWarning() << "no strings";
        return false;
    }
    QString sname = st.at(0);
    QString sversion = st.at(1);
    QString shardware = st.at(2);

    trace()->blocks(st);

    QStringList fnames = stream.read_strings(ident.flags.bits.files);
    for (auto i : fnames) {
        if (!i.isEmpty())
            continue;
        fnames.clear();
        break;
    }
    if (fnames.isEmpty()) {
        qWarning() << "no files";
        return false;
    }
    trace()->blocks(fnames);

    if (stream.available() > 0) {
        qWarning() << "corrupted ident_s";
        return false;
    }

    fnames.sort();

    _node->setTitle(sname);
    _node->updateFiles(fnames);

    // node descr
    QStringList descr;
    descr.append(shardware);
    if (sversion != App::version())
        descr.append(sversion);
    if (ident.flags.bits.files == 1 && fnames.contains("fw"))
        descr.append("LOADER");
    _node->setDescr(descr.join(' '));

    QJsonObject json;
    json.insert("uid", _node->uid());
    json.insert("name", sname);
    json.insert("version", sversion);
    json.insert("hardware", shardware);

    json.insert("reconf", ident.flags.bits.reconf ? true : false);
    json.insert("busy", ident.flags.bits.busy ? true : false);

    QJsonArray array;
    for (auto i : fnames)
        array.append(i);
    json.insert("files", array);

    _node->identReceived(json);

    return true;
}

void PApxNodeRequestFile::request(PApxRequest &req)
{
    req << _op;
    trace()->block(QString::number(_op));
    req.write_string(_name.toUtf8());
    trace()->block(_name);
}
