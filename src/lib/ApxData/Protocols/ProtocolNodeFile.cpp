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
#include "ProtocolNodeFile.h"
#include "ProtocolNode.h"
#include "ProtocolNodes.h"

#include <crc.h>

#include <App/AppLog.h>
#include <App/AppRoot.h>
#include <Mandala/Mandala.h>

ProtocolNodeFile::ProtocolNodeFile(ProtocolNode *node, const QString &name)
    : ProtocolBase(node, name)
    , node(node)
{
    setIcon("paperclip");
    memset(&_info, 0, sizeof(_info));

    connect(this, &ProtocolNodeFile::error, this, &ProtocolNodeFile::reset);
}

void ProtocolNodeFile::upload(QByteArray data, xbus::node::file::offset_t offset)
{
    reset();
    _op_data.swap(data);
    _op_offset = offset;
    _op_size = static_cast<xbus::node::file::size_t>(_op_data.size());
    _op_tcnt = 0;
    setValue(tr("Uploading"));
    setActive(true);
    request(xbus::node::file::wopen)->schedule();
}
void ProtocolNodeFile::write_next()
{
    if (_op_tcnt == _op_size) {
        request(xbus::node::file::close)->schedule();
        return;
    }
    size_t tail = _op_size - _op_tcnt;
    size_t sz = 256;
    if (sz > tail)
        sz = tail;

    updateProgress();

    const QByteArray &ba = _op_data.mid(static_cast<int>(_op_tcnt), static_cast<int>(sz));
    if (static_cast<int>(sz) != ba.size()) {
        qWarning() << "block: " << sz << ba.size();
        stop();
    }

    ProtocolNodeRequest *req = request(xbus::node::file::write);
    req->write<xbus::node::file::offset_t>(_op_offset);
    sz = req->write(ba.data(), sz);
    _op_hash = apx::crc32(ba.data(), sz);
    req->schedule();
    qDebug() << _op_tcnt << sz << QString::number(_op_hash, 16);

    setValue(QString("%1kB/%2kB").arg(_op_tcnt / 1024).arg(_op_size / 1024.0, 0, 'f', 1));
}
bool ProtocolNodeFile::resp_write(ProtocolStreamReader &stream)
{
    if (stream.available() < sizeof(xbus::node::file::offset_t))
        return false;
    xbus::node::file::offset_t offset;
    stream >> offset;
    if (offset != _op_offset) {
        qWarning() << "offset: " << QString::number(offset, 16) << QString::number(_op_offset, 16);
        // response offset mismatch - don't interrupt
        return true;
    }

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
    if (hash != _op_hash) {
        qWarning() << "hash: " << QString::number(hash, 16) << QString::number(_op_hash, 16);
        return false;
    }

    if (stream.available() > 0)
        return false;

    //qDebug() << _req;
    ack_req();

    _op_offset += size;
    _op_tcnt += size;
    if (_op_tcnt > _op_size)
        return false;

    write_next();
    return true;
}

void ProtocolNodeFile::download()
{
    reset();
    _op_offset = 0;
    _op_size = 0;
    _op_tcnt = 0;

    setValue(tr("Downloading"));
    setActive(true);
    request(xbus::node::file::ropen)->schedule();
}
void ProtocolNodeFile::read_next()
{
    setValue(QString("%1kB/%2kB").arg(_op_tcnt / 1024).arg(_op_size / 1024.0, 0, 'f', 1));

    //qDebug() << _op_offset << _op_tcnt << _op_size;
    if (_op_tcnt == _op_size) {
        //all data read
        qDebug() << "done";
        request(xbus::node::file::close)->schedule();
        return;
    }

    updateProgress();

    ProtocolNodeRequest *req = request(xbus::node::file::read);
    req->write<xbus::node::file::offset_t>(_op_offset);
    req->schedule();
}
bool ProtocolNodeFile::resp_read(ProtocolStreamReader &stream)
{
    if (stream.available() <= sizeof(xbus::node::file::offset_t))
        return false;
    xbus::node::file::offset_t offset;
    stream >> offset;
    if (offset != _op_offset) { //just skip non-sequental
        qWarning() << offset << _op_offset;
        return true;
    }

    ack_req();

    size_t size = stream.available();
    _op_offset += size;
    _op_tcnt += size;

    //qDebug() << _op_offset << _op_tcnt << _op_size;

    if (_op_tcnt > _op_size)
        return false;

    _op_data.append(stream.payload());
    read_next();
    return true;
}

void ProtocolNodeFile::stop()
{
    setActive(false);
    if (_op != xbus::node::file::idle) {
        qWarning() << "interrupted:" << name() << _op;
        reset();
        emit interrupted();
        emit finished();
        return;
    }
    reset();
}

void ProtocolNodeFile::updateProgress()
{
    size_t v = _op_size > 0 ? _op_tcnt * 100 / _op_size : 0;
    setProgress(static_cast<int>(v));
}

void ProtocolNodeFile::downlink(xbus::node::file::op_e op, ProtocolStreamReader &stream)
{
    //qDebug() << name() << op << stream.available();
    switch (op) {
    default:
        break;
    case xbus::node::file::info:
        if (!check_info(stream))
            break;
        ack_req();
        return;

    case xbus::node::file::ropen:
        if (!check_info(stream))
            break;
        if (!(_info.flags.bits.readable && _info.flags.bits.oread))
            break;
        if (!check_op(op))
            break;
        ack_req();
        _op_size = _info.size;
        if (_op_offset == 0)
            _op_offset = _info.offset;
        if (_op_tcnt == 0) {
            _op_hash = 0;
            read_next();
        }
        return;
    case xbus::node::file::wopen:
        if (!check_info(stream))
            break;
        if (!(_info.flags.bits.writable && _info.flags.bits.owrite))
            break;
        if (!check_op(op))
            break;
        ack_req();
        if (_op_tcnt == 0) {
            _op_hash = 0;
            write_next();
        }
        return;
    case xbus::node::file::close:
        if (!check_info(stream))
            break;
        if (!check_op(op))
            break;

        if (_op_tcnt == _op_size) {
            qDebug() << "transfer complete";
            setActive(false);
            if (_info.flags.bits.owrite)
                emit uploaded();
            else if (_info.flags.bits.oread)
                emit downloaded(_info, _op_data);
        }
        reset();
        stop();
        //reply_info(op);
        return;
    case xbus::node::file::read:
        if (!_info.flags.bits.oread)
            break;
        if (!check_op(op))
            break;
        if (!resp_read(stream))
            break;
        return;
    case xbus::node::file::write:
        if (!_info.flags.bits.owrite)
            break;
        if (!check_op(op))
            break;
        if (!resp_write(stream))
            break;
        return;
    }
    // error
    emit error();
    //reply_info(xbus::node::file::abort);
}

ProtocolNodeRequest *ProtocolNodeFile::request(xbus::node::file::op_e op)
{
    ProtocolNodeRequest *req = node->request(mandala::cmd::env::nmt::file::uid);
    *req << op;
    req->write_string(name().toUtf8());
    _op = op;
    _req = req;
    connect(req, &ProtocolNodeRequest::timeout, this, &ProtocolNodeFile::error);
    //qDebug() << op << req->dump();
    return req;
}

void ProtocolNodeFile::reset()
{
    //qDebug() << _op;
    _op = xbus::node::file::idle;

    _op_data.clear();

    setValue(QVariant());
    setProgress(-1);
    ack_req();
    setActive(false);
}

bool ProtocolNodeFile::check_info(ProtocolStreamReader &stream)
{
    if (stream.available() != xbus::node::file::info_s::psize())
        return false;
    xbus::node::file::info_s info;
    info.read(&stream);
    bool chg = true;
    do {
        if (info.size != _info.size)
            break;
        if (info.time != _info.time)
            break;
        if (info.hash != _info.hash)
            break;
        if (info.offset != _info.offset)
            break;
        if (info.flags.bits.readable != _info.flags.bits.readable)
            break;
        if (info.flags.bits.writable != _info.flags.bits.writable)
            break;
        chg = false;
    } while (0);
    _info = info;
    if (chg) {
        qDebug() << "info update";
        QStringList st;
        st << AppRoot::capacityToString(info.size, 2);
        st << QString("T:%1").arg(info.time);
        st << QString("H:%1").arg(info.hash, 8, 16, QChar('0')).toUpper();
        st << QString("O:%1").arg(info.offset, 8, 16, QChar('0')).toUpper();
        st << QString("%1%2")
                  .arg(info.flags.bits.readable ? 'R' : ' ')
                  .arg(info.flags.bits.writable ? 'W' : ' ');
        setDescr(st.join(' '));
    }
    return true;
}

bool ProtocolNodeFile::check_op(xbus::node::file::op_e op)
{
    if (_op != op) {
        qWarning() << _op << op;
        return false;
    }
    return true;
}

void ProtocolNodeFile::ack_req()
{
    if (!_req)
        return;
    _req->finish(true);
    _req = nullptr;
}
