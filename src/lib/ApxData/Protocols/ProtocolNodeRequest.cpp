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

ProtocolNodeRequest::ProtocolNodeRequest(ProtocolNodes *nodes,
                                         const QString &sn,
                                         const xbus::pid_s &pid,
                                         size_t retry_cnt,
                                         size_t timeout_ms)
    : QObject(nodes)
    , ProtocolStreamWriter(packet_buf, sizeof(packet_buf))
    , nodes(nodes)
    , node(nodes->getNode(sn, false))
    , _uid(pid.uid)
    , retry_cnt(retry_cnt)
    , timeout_ms(timeout_ms > 0 ? timeout_ms : 500)
{
    //repare stream
    pid.write(this);
    if (!sn.isEmpty()) {
        QByteArray src(QByteArray::fromHex(sn.toUtf8()));
        size_t sz = static_cast<size_t>(src.size());
        if (sz > sizeof(xbus::node::guid_t)) {
            sz = sizeof(xbus::node::guid_t);
            qWarning() << "guid size:" << sz;
        }
        xbus::node::guid_t guid;
        memset(guid, 0, sizeof(guid));
        memcpy(guid, src.data(), sz);
        write(guid, sizeof(guid));
    }
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
    if (active)
        return true;
    return QString::compare(toByteArray().toHex(), other->toByteArray().toHex()) < 0;
}
void ProtocolNodeRequest::acknowledge()
{
    finish(true);
}
void ProtocolNodeRequest::extend(size_t ms)
{
    if (timeout_ms > ms)
        ms = timeout_ms;
    if (active && timeout_ms > 0) {
        timer.start(static_cast<int>(ms));
    }
}
bool ProtocolNodeRequest::equals(xbus::node::crc_t crc)
{
    if (!_crc_valid) {
        _crc = get_crc(packet_buf, pos());
        _crc_valid = true;
    }
    return _crc == crc;
}

void ProtocolNodeRequest::schedule()
{
    nodes->schedule(this);
}

void ProtocolNodeRequest::trigger()
{
    active = true;
    //qDebug()<<cmd<<sn<<data.size()<<data.toHex().toUpper();
    nodes->sendRequest(this);
    timer.start(static_cast<int>(timeout_ms));
}

void ProtocolNodeRequest::finish(bool acknowledged)
{
    //qDebug() << dump();
    this->acknowledged = acknowledged;
    timer.stop();
    emit finished(this);
}

void ProtocolNodeRequest::triggerTimeout()
{
    timer.stop();
    if (retry_cnt == 0) {
        finish();
        return;
    }
    //request timeout
    if (retry < retry_cnt) {
        retry++;
        qDebug() << retry;
        emit retrying(retry, retry_cnt);
        trigger();
        return;
    }
    if (retry_cnt > 0) {
        apxConsoleW() << tr("Service timeout")
                      << QString("(%1): %2 %3")
                             .arg(node ? node->name() : "?")
                             .arg(Mandala::meta(_uid).name)
                             .arg(dump(stream_pos_s));
        emit timeout();
    }
    finish();
}
