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
    // schedule delayed to avoid consequent requests duplicate deletions by uid
    QTimer::singleShot(1, node, [this]() { _node->schedule_request(this); });
}

PTrace *PApxNodeRequest::trace() const
{
    return _node->trace();
}
void PApxNodeRequest::discard()
{
    _node->delete_request(uid());
}

bool PApxNodeRequest::make_request(PApxRequest &req)
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

    return request(req);
}

bool PApxNodeRequestReboot::request(PApxRequest &req)
{
    req << _type;
    return true;
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
    json.insert("hash", (qint64) ident.hash);
    json.insert("format", (qint64) ident.format);

    json.insert("reconf", ident.flags.bits.reconf ? true : false);
    json.insert("busy", ident.flags.bits.busy ? true : false);

    QJsonArray array;
    for (auto i : fnames)
        array.append(i);
    json.insert("files", array);

    _node->identReceived(json);

    return true;
}

bool PApxNodeRequestFile::request(PApxRequest &req)
{
    req << _op;
    trace()->block(QString::number(_op));
    req.write_string(_name.toUtf8());
    trace()->block(_name);
    return true;
}

bool PApxNodeRequestFileRead::request(PApxRequest &req)
{
    PApxNodeRequestFile::request(req);

    switch (_op) {
    default:
        return false;
    case xbus::node::file::ropen:
        break;
    case xbus::node::file::close:
        break;
    case xbus::node::file::read:
        req.write<xbus::node::file::offset_t>(_offset);
        break;
    }
    return true;
}
bool PApxNodeRequestFileRead::response(PStreamReader &stream)
{
    //qDebug() << "re:" << _op << stream.available();
    if (stream.available() <= sizeof(xbus::node::file::op_e))
        return false;

    xbus::node::file::op_e op;
    stream >> op;

    if (!(op & xbus::node::file::reply_op_mask))
        return false;

    op = static_cast<xbus::node::file::op_e>(op & ~xbus::node::file::reply_op_mask);

    if (op == xbus::node::file::abort) {
        qWarning() << "transfer aborted by node";
        reset();
        return true;
    }

    if (op != _op)
        return false;

    const char *s = stream.read_string(16);
    if (!s || QString(s) != _name)
        return false;

    //qDebug() << _name << op << _op;

    switch (op) {
    default:
        return false;
    case xbus::node::file::ropen:
        if (stream.available() != xbus::node::file::info_s::psize())
            return false;
        reset();
        _info.read(&stream);
        _offset = _info.offset;
        read_next();
        return false;
    case xbus::node::file::close:
        if (_info.size == _tcnt && _info.size > 0) {
            if (_hash != _info.hash) {
                qWarning() << "read error:" << _name;
                qWarning() << "hash: " << QString::number(_hash, 16)
                           << QString::number(_info.hash, 16);
            } else {
                //qDebug() << "read ok:" << _name << QString::number(_hash, 16);
            }
        }
        reset();
        break;
    case xbus::node::file::read: {
        if (stream.available() <= sizeof(xbus::node::file::offset_t))
            return false;

        xbus::node::file::offset_t offset;
        stream >> offset;
        if (offset != _offset) { //just skip non-sequental
            qWarning() << "offset:" << offset << _offset;
            return true;
        }

        //qDebug() << "rd:" << offset;

        size_t size = stream.available();
        _offset += size;
        _tcnt += size;

        if (_tcnt > _info.size) {
            qWarning() << "overflow:" << _tcnt << _info.size;
            return true;
        }
        _hash = apx::crc32(stream.ptr(), size, _hash);
        read_next();
        return false;
    }
    }

    return true;
}
void PApxNodeRequestFileRead::reset()
{
    _op = xbus::node::file::ropen;
    _info = {};
    _offset = 0;
    _tcnt = 0;
    _hash = 0xFFFFFFFF;
}
void PApxNodeRequestFileRead::read_next()
{
    if (_tcnt == _info.size) {
        //all data read
        //qDebug() << "done";
        _op = xbus::node::file::close;
        _node->reschedule_request(this);
        return;
    }
    _op = xbus::node::file::read;
    _node->reschedule_request(this);
}

bool PApxNodeRequestUpdate::request(PApxRequest &req)
{
    if (_index >= _values.size()) {
        // all written - request to save
        _fid = 0xFFFFFFFF;
        req << _fid;
        trace()->block("SAVE");
        return true;
    }

    auto name = _values.keys().at(_index);
    xbus::node::conf::type_e type;
    if (!_node->find_field(name, &_fid, &type)) {
        qWarning() << "no field:" << name;
        return false;
    }

    req << _fid;
    trace()->block(QString::number(_fid >> 8));
    trace()->block(QString::number(_fid & 0xFF));

    QVariant value = _values.value(name);
    _node->write_param(req, type, value);
    trace()->block(value.toString());

    _index++;

    return true;
}
bool PApxNodeRequestUpdate::response(PStreamReader &stream)
{
    if (stream.available() != sizeof(xbus::node::conf::fid_t))
        return false;
    trace()->data(stream.payload());

    xbus::node::conf::fid_t fid;
    stream >> fid;

    // qDebug() << fid << _fid;

    if (fid != _fid) {
        // qWarning() << "fid:" << fid << _fid;
        return false;
    }

    if (_index >= _values.size() && _fid == 0xFFFFFFFF) {
        // qDebug() << "saved";
        _node->confSaved();
        return true;
    }

    _node->reschedule_request(this);
    return false;
}
