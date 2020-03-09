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
#include "ProtocolNodeRequest.h"
#include "ProtocolNode.h"
#include "ProtocolNodes.h"

#include <Mandala/Mandala.h>
#include <crc/crc.h>

#include <App/AppLog.h>

ProtocolNodeRequest::ProtocolNodeRequest(
    ProtocolNodes *nodes, const QString &sn, xbus::pid_t pid, int timeout_ms, int retry_cnt)
    : QObject(nodes)
    , ProtocolStreamWriter(packet_buf, sizeof(packet_buf))
    , nodes(nodes)
    , node(nodes->getNode(sn, false))
    , timeout_ms(timeout_ms)
    , retry_cnt(retry_cnt < 0 ? (timeout_ms > 0 ? 4 : 0) : retry_cnt)
{
    //repare stream
    write<xbus::pid_t>(pid);
    xbus::node::guid_t guid;
    memset(guid, 0, sizeof(guid));
    if (!sn.isEmpty()) {
        QByteArray src(QByteArray::fromHex(sn.toUtf8()));
        size_t sz = static_cast<size_t>(src.size());
        if (sz > sizeof(guid)) {
            sz = sizeof(guid);
            qWarning() << "guid size:" << sz;
        }
        memcpy(guid, src.data(), sz);
    }
    write(guid, sizeof(guid));
    stream_pos_s = pos();

    connect(&timer, &QTimer::timeout, this, &ProtocolNodeRequest::triggerTimeout);
}

xbus::node::crc_t ProtocolNodeRequest::get_crc(const void *data, size_t sz)
{
    return CRC_16_APX(data, sz, 0);
}

bool ProtocolNodeRequest::equals(const ProtocolNodeRequest *other)
{
    return equals(other->_crc);
}
bool ProtocolNodeRequest::lessThan(const ProtocolNodeRequest *other)
{
    return QString::compare(toByteArray().toHex(), other->toByteArray().toHex()) < 0;
}
bool ProtocolNodeRequest::acknowledge(xbus::node::crc_t crc)
{
    if (!equals(crc))
        return false;
    finish(true);
    return true;
}
bool ProtocolNodeRequest::equals(xbus::node::crc_t crc)
{
    if (!_crc_valid) {
        _crc = get_crc(packet_buf, pos());
        _crc_valid = true;
    }
    return _crc == crc;
}

void ProtocolNodeRequest::trigger()
{
    active = true;
    //qDebug()<<cmd<<sn<<data.size()<<data.toHex().toUpper();
    nodes->sendRequest(this);
    if (timeout_ms) {
        timer.start(timeout_ms);
    } else {
        finish();
    }
}

void ProtocolNodeRequest::finish(bool acknowledged)
{
    //qDebug()<<cmd<<sn<<data.size();//<<t.elapsed();//data.toHex().toUpper();
    this->acknowledged = acknowledged;
    timer.stop();
    emit finished(this);
    active = false;
}

void ProtocolNodeRequest::triggerTimeout()
{
    timer.stop();
    //request timeout
    if (retry < retry_cnt) {
        retry++;
        emit retrying(retry, retry_cnt);
        trigger();
        return;
    }
    active = false;
    if (retry_cnt > 0) {
        apxConsoleW() << tr("Service timeout")
                      << QString("(%1): %2 %3")
                             .arg(node ? node->ident().name : "?")
                             .arg(Mandala::meta(pid).name)
                             .arg(dump(stream_pos_s));
        emit timeout();
    }
    finish();
}
