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
#include "PApxNodeFile.h"
#include "PApxNode.h"
#include "PApxNodes.h"

#include <crc.h>

#include <App/AppLog.h>
#include <App/AppRoot.h>
#include <Mandala/Mandala.h>

#include <xbus/XbusNode.h>

PApxNodeFile::PApxNodeFile(PApxNode *node, const QString &name)
    : PTreeBase(node, name, name)
    , _node(node)
{
    connect(this, &Fact::triggered, this, &PApxNodeFile::requestInfo);
}

void PApxNodeFile::process_downlink(xbus::node::file::op_e op, PStreamReader &stream)
{
    // collect and build file content from received read/write parts and emit signal with data

    bool is_response = op & xbus::node::file::reply_op_mask;
    op = static_cast<xbus::node::file::op_e>(op & ~xbus::node::file::reply_op_mask);

    switch (op) {
    default:
        break;
    case xbus::node::file::info:
        if (!is_response)
            return;
        if (!check_info(stream))
            break; // info changed
        return;

    case xbus::node::file::ropen:
        if (!is_response)
            return;
        check_info(stream);

        if (!(_info.flags.bits.readable && _info.flags.bits.oread))
            break;

        reset();
        _size = _info.size;
        _offset = _info.offset;
        _open_op = op;
        //qDebug() << "opened read" << _size << _offset;
        return;

    case xbus::node::file::wopen:
        if (!is_response)
            return;
        check_info(stream);

        if (!(_info.flags.bits.writable && _info.flags.bits.owrite))
            break;

        reset();
        _size = 65536 * 1024;
        _offset = _info.offset;
        _open_op = op;
        //qDebug() << "opened write" << _offset;
        return;

    case xbus::node::file::close:
        if (!is_response)
            return;
        if (_open_op == xbus::node::file::ropen) {
            if (!check_info(stream))
                break;
        } else if (_open_op == xbus::node::file::wopen) {
            // uploads from another GCS
            check_info(stream);
            _size = _data.size();
            _info.size = _size;
            _info.hash = _hash;
            if (_size == 0)
                break;
        } else
            break;

        if (_size == 0 && _info.size == 0 && _data.size() == 0) {
            qDebug() << "download ok:" << name() << "empty";
        } else {
            // check downloaded data
            if (_hash != _info.hash || _tcnt != _size || _data.size() != _tcnt) {
                qWarning() << "download error:" << name() << _data.size();
                qWarning() << "hash: " << QString::number(_hash, 16)
                           << QString::number(_info.hash, 16);
                break;
            }
            qDebug() << "download ok:" << name() << _size << "bytes" << QString::number(_hash, 16);
        }

        if (_open_op == xbus::node::file::ropen)
            emit downloaded(_node, _info, _data);
        else
            emit uploaded(_node, _info, _data);

        break;

    case xbus::node::file::read:
        if (!is_response)
            return;
        if (!_info.flags.bits.oread)
            break;
        if (!_size || _open_op != xbus::node::file::ropen)
            break;
        if (!read(stream))
            break;
        return;

    case xbus::node::file::write:
        if (is_response)
            return;
        if (!_info.flags.bits.owrite)
            break;
        if (!_size || _open_op != xbus::node::file::wopen)
            break;
        if (!read(stream))
            break;
        return;

    case xbus::node::file::extend:
        return;
    case xbus::node::file::abort:
        qWarning() << "abort response";
        break;
    }
    // error
    reset();
}

bool PApxNodeFile::read(PStreamReader &stream)
{
    if (stream.available() <= sizeof(xbus::node::file::offset_t))
        return false;

    xbus::node::file::offset_t offset;
    stream >> offset;
    if (offset != _offset) { //just skip non-sequental
        return true;
    }

    size_t size = stream.available();
    _offset += size;
    _tcnt += size;

    //qDebug() << "rd:" << offset << size;

    if (_tcnt > _size)
        return false;

    _hash = apx::crc32(stream.ptr(), size, _hash);

    _data.append(stream.payload());
    return true;
}

void PApxNodeFile::reset()
{
    _size = _tcnt = _offset = 0;
    _hash = 0xFFFFFFFF;

    _data.clear();

    _open_op = xbus::node::file::idle;

    setValue(QVariant());
    setProgress(-1);
}

bool PApxNodeFile::check_info(PStreamReader &stream)
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
    if (!chg)
        return true;
    {
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
    return false;
}

void PApxNodeFile::updateProgress()
{
    size_t v = _size > 0 ? _tcnt * 100 / _size : 0;
    setProgress(static_cast<int>(v));
}
