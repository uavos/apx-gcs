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
#include <XbusNode.h>
#include <XbusScript.h>

#include <crc.h>

PApxNodeRequest::PApxNodeRequest(PApxNode *node, mandala::uid_t uid, uint timeout_ms)
    : QObject()
    , _node(node)
    , _uid(uid)
    , _timeout_ms(timeout_ms)
{
    if (!uid)
        return;
    // schedule delayed to avoid consequent requests duplicate deletions by uid
    QTimer::singleShot(1, node, [this]() { _node->schedule_request(this); });
}
PApxNodeRequest::~PApxNodeRequest()
{
    //qDebug() << title();
    _node->request_deleted(this);
}
QString PApxNodeRequest::title() const
{
    return QString("%1/%2/%3")
        .arg(_node->parent()->parent()->title())
        .arg(_node->title())
        .arg(Mandala::meta(uid()).name);
}

PTrace *PApxNodeRequest::trace()
{
    return _node->trace();
}
void PApxNodeRequest::discard()
{
    _node->delete_request(this);
}

bool PApxNodeRequest::make_request(PApxRequest &req)
{
    _active = true;

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
    trace()->block(_node->title().append(':'));
    trace()->tree();

    return request(req);
}
bool PApxNodeRequest::check_response(PStreamReader &stream)
{
    auto pos_s = stream.pos();
    auto ret = response(stream);
    stream.reset(pos_s);
    return ret;
}

bool PApxNodeRequestReboot::request(PApxRequest &req)
{
    req << _type;
    return true;
}

bool PApxNodeRequestMod::request(PApxRequest &req)
{
    switch (_cmd) {
    default:
        qWarning() << "wrong mod request" << _cmd << _adr.toHex().toUpper() << _data;
        return false;
    case PNode::sh:
        _op = xbus::node::mod::sh;
        _timeout_ms = 0;
        break;
    case PNode::ls:
        _op = xbus::node::mod::ls;
        break;
    case PNode::status:
        _op = xbus::node::mod::status;
        break;
    }

    req << _op;
    trace()->block(QString::number(_op));

    if (!_adr.isEmpty()) {
        req.append(_adr);
        trace()->data(_adr);
    }

    if (!_data.isEmpty()) {
        for (auto const &s : _data) {
            req.write_string(s.toUtf8().data());
        }
        trace()->blocks(_data);
    }
    return true;
}
bool PApxNodeRequestMod::response(PStreamReader &stream)
{
    if (!_active)
        return false;

    if (stream.available() < sizeof(xbus::node::mod::op_e)) {
        qWarning() << "size" << stream.available();
        return false;
    }
    xbus::node::mod::op_e op;
    stream >> op;

    if (op != _op)
        return false;

    trace()->block(QString::number(op));

    PNode::mod_cmd_e cmd;

    switch (op) {
    default:
        return false;
    case xbus::node::mod::ls:
        cmd = PNode::ls;
        break;
    case xbus::node::mod::status:
        cmd = PNode::status;
        break;
    }
    // read module adr
    QByteArray adr;
    uint8_t n = 0;
    while (stream.available() > 0) {
        stream >> n;
        adr.append(static_cast<char>(n));
        if (n == 0xFF)
            break;
    }
    if (n != 0xFF || adr.isEmpty()) {
        qWarning() << "bad adr";
        return false;
    }
    adr.chop(1);
    if (adr != _adr) // reply for other request
        return false;
    trace()->data(adr);

    auto data = stream.read_strings();
    trace()->blocks(data);

    _node->modReceived(cmd, adr, data);

    return true;
}

bool PApxNodeRequestUsr::request(PApxRequest &req)
{
    req.write<quint8>(_cmd);
    req.append(_data);
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
        // return false;
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

    QVariantMap m;
    m.insert("uid", _node->uid());
    m.insert("name", sname);
    m.insert("version", sversion);
    m.insert("hardware", shardware);
    m.insert("hash", _node->hashToText(ident.hash));
    m.insert("format", ident.format);

    m.insert("reconf", ident.flags.bits.reconf ? true : false);
    m.insert("busy", ident.flags.bits.busy ? true : false);

    QVariantList array;
    for (auto i : fnames)
        array.append(i);
    m.insert("files", array);

    _node->identReceived(m);

    return true;
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
    if (type == xbus::node::conf::option)
        value = _node->textToOption(value, _fid >> 8);
    else if (type == xbus::node::conf::bind)
        value = _node->stringToMandala(value.toString());
    _node->write_param(req, type, value);
    trace()->block(value.toString());

    _index++;

    return true;
}
bool PApxNodeRequestUpdate::response(PStreamReader &stream)
{
    if (!_active)
        return false;

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

    if (_index >= _values.size()) {
        if (_fid == 0xFFFFFFFF) {
            // qDebug() << "saved";
            return true;
        }
    }

    _node->reschedule_request(this);
    return false;
}

bool PApxNodeRequestFile::request(PApxRequest &req)
{
    if (!_node->file(_name)) {
        qWarning() << "no file" << _name;
        return false;
    }
    req << _op;
    trace()->block(QString::number(_op));
    req.write_string(_name.toUtf8());
    trace()->block(_name);

    if (_op == xbus::node::file::close)
        return true;
    if (_op == _op_init)
        return true;
    if (_op != _op_file)
        return false;

    req.write<xbus::node::file::offset_t>(_offset);

    return request_file(req);
}
void PApxNodeRequestFile::reset()
{
    _op = _op_init;
    _info = {};
    _offset = 0;
    _tcnt = 0;
    _hash = 0xFFFFFFFF;
}
bool PApxNodeRequestFile::response(PStreamReader &stream)
{
    if (!_active)
        return false;

    //qDebug() << "re:" << _op << stream.available();
    if (stream.available() <= sizeof(xbus::node::file::op_e))
        return false;

    xbus::node::file::op_e op;
    stream >> op;

    if (!(op & xbus::node::file::reply_op_mask))
        return false;

    const char *s = stream.read_string(16);
    if (!s || QString(s) != _name)
        return false;

    op = static_cast<xbus::node::file::op_e>(op & ~xbus::node::file::reply_op_mask);

    if (op == xbus::node::file::abort) {
        qWarning() << "transfer aborted by node";
        reset();
        return true;
    }
    if (op == xbus::node::file::extend) {
        if (_op != xbus::node::file::write || !_info.flags.bits.owrite)
            return false;
        if (stream.available() < sizeof(xbus::node::file::offset_t))
            return false;
        xbus::node::file::offset_t offset;
        stream >> offset;
        if (offset != _offset) {
            qWarning() << "ext offset: " << QString::number(offset, 16)
                       << QString::number(_offset, 16);
            // response offset mismatch - don't interrupt
            return false;
        }

        if (stream.available() != sizeof(xbus::node::file::timeout_t))
            return false;
        xbus::node::file::timeout_t time;
        stream >> time;
        if (time == 0) {
            qWarning() << "ext time: " << time;
            return false;
        }
        _node->extend_request(this, time);

        return false;
    }

    if (op != _op)
        return false;

    //qDebug() << _name << op << _op;
    if (op == _op_init) {
        if (stream.available() != xbus::node::file::info_s::psize())
            return false;
        reset();
        _info.read(&stream);
        _offset = _info.offset;
        opened();
        next();
        return false;
    }
    if (op == xbus::node::file::close) {
        if (_info.size == _tcnt) {
            if (_info.size > 0 && _hash != _info.hash && _op_file == xbus::node::file::read) {
                qWarning() << "transfer error:" << _name;
                qWarning() << "hash: " << QString::number(_hash, 16)
                           << QString::number(_info.hash, 16);
                reset();
                return false;
            }
            //qDebug() << "transfer ok:" << _name << QString::number(_hash, 16);
            if (_op_file == xbus::node::file::read)
                emit downladed();
            else
                emit uploaded();
        }
        reset();
        return true;
    }
    if (op != _op_file)
        return false;

    if (stream.available() <= sizeof(xbus::node::file::offset_t))
        return false;

    xbus::node::file::offset_t offset;
    stream >> offset;
    if (offset != _offset) { //just skip non-sequental
        qWarning() << "offset:" << QString::number(offset, 16) << QString::number(_offset, 16);
        return false;
    }

    return response_file(offset, stream);
}
void PApxNodeRequestFile::next()
{
    if (_tcnt == _info.size) {
        //all data read
        //qDebug() << "done";
        _op = xbus::node::file::close;
        _node->reschedule_request(this);
        return;
    }
    _op = _op_file;
    _node->reschedule_request(this);
}

bool PApxNodeRequestFileRead::response_file(xbus::node::file::offset_t offset, PStreamReader &stream)
{
    size_t size = stream.available();
    _offset += size;
    _tcnt += size;

    if (_tcnt > _info.size) {
        qWarning() << "overflow:" << _tcnt << _info.size;
        return true;
    }
    _hash = apx::crc32(stream.ptr(), size, _hash);

    size_t v = _info.size > 0 ? _tcnt * 100 / _info.size : 0;
    _node->setProgress(static_cast<int>(v));
    emit progress(v);

    next();
    return false;
}

void PApxNodeRequestFileWrite::opened()
{
    _info.size = _data.size();
    _offset = _woffset;
}
bool PApxNodeRequestFileWrite::request_file(PApxRequest &req)
{
    // write data part
    size_t tail = _info.size - _tcnt;
    size_t sz = 256;
    if (sz > tail)
        sz = tail;

    const QByteArray &ba = _data.mid(static_cast<int>(_tcnt), static_cast<int>(sz));
    if (static_cast<int>(sz) != ba.size()) {
        qWarning() << "block size: " << sz << ba.size();
        return false;
    }
    sz = req.write(ba.data(), sz);
    _hash = apx::crc32(ba.data(), sz);
    return true;
}
bool PApxNodeRequestFileWrite::response_file(xbus::node::file::offset_t offset,
                                             PStreamReader &stream)
{
    if (stream.available() < sizeof(xbus::node::file::size_t))
        return false;

    xbus::node::file::size_t size;
    stream >> size;
    if (size == 0) {
        qWarning() << "size: " << size;
        return false;
    }

    if (stream.available() < sizeof(xbus::node::hash_t))
        return false;

    xbus::node::hash_t hash;
    stream >> hash;

    if (hash != _hash) {
        qWarning() << "hash: " << QString::number(hash, 16) << QString::number(_hash, 16);
        return false;
    }

    if (stream.available() > 0)
        return false;

    _offset += size;
    _tcnt += size;
    if (_tcnt > _info.size)
        return false;

    size_t v = _data.size() > 0 ? _tcnt * 100 / _data.size() : 0;
    _node->setProgress(static_cast<int>(v));
    emit progress(v);

    next();
    return false;
}
