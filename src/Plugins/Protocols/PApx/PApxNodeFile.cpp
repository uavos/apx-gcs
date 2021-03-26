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
    //qDebug() << name() << op << stream.available();
    switch (op) {
    default:
        break;
    case xbus::node::file::info:
        if (!check_info(stream))
            break; // info changed
        return;

    case xbus::node::file::ropen:
        check_info(stream);
        if (!(_info.flags.bits.readable && _info.flags.bits.oread))
            break;

        reset();
        _size = _info.size;
        return;
    case xbus::node::file::wopen:
        if (!check_info(stream))
            break;
        if (!(_info.flags.bits.writable && _info.flags.bits.owrite))
            break;

        reset();
        _size = _info.size;
        return;
    case xbus::node::file::close:
        if (!check_info(stream))
            break;
        reset();
        return;
    case xbus::node::file::read:
        if (!_info.flags.bits.oread)
            break;
        // if (!resp_read(stream))
        //     break;
        return;
    case xbus::node::file::write:
        if (!_info.flags.bits.owrite)
            break;
        // if (!resp_write(stream))
        //     break;
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

void PApxNodeFile::reset()
{
    _size = _tcnt = _offset = 0;
    _hash = 0;

    _data.clear();

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
