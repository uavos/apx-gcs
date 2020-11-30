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
#include "ProtocolNodeRequest.h"
#include "ProtocolNode.h"
#include "ProtocolNodes.h"

#include <Mandala/Mandala.h>

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
    , _pid(pid)
    , _sn(sn)
    , retry_cnt(retry_cnt)
    , timeout_ms(timeout_ms > 0 ? timeout_ms : 1500)
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

bool ProtocolNodeRequest::equals(const ProtocolNodeRequest *other)
{
    if (other->_sn != _sn)
        return false;
    if (other->_pid.uid != _pid.uid)
        return false;
    if (other->_pid.pri != _pid.pri)
        return false;
    if (other->pos() != pos())
        return false;
    if (other->stream_pos_s != stream_pos_s)
        return false;
    if (memcmp(other->buffer() + stream_pos_s, buffer() + stream_pos_s, pos() - stream_pos_s) != 0)
        return false;

    qDebug() << "duplicate req"
             << QString("(%1): %2 %3")
                    .arg(node ? node->name() : "?")
                    .arg(Mandala::meta(_pid.uid).name)
                    .arg(dump(stream_pos_s));
    return true;
}
bool ProtocolNodeRequest::lessThan(const ProtocolNodeRequest *other)
{
    if (active)
        return true;
    return QString::compare(toByteArray().toHex(), other->toByteArray().toHex()) < 0;
}

void ProtocolNodeRequest::extend(size_t ms)
{
    qDebug() << ms;
    if (timeout_ms > ms)
        ms = timeout_ms;
    if (active && timeout_ms > 0) {
        timer.start(static_cast<int>(ms));
    }
}
bool ProtocolNodeRequest::equals(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    if (pid.uid != _pid.uid)
        return false;
    if (pid.seq != _pid.seq)
        return false;
    if (stream.pos() > pos())
        return false;
    size_t pad = xbus::pid_s::psize();
    if (memcmp(stream.buffer() + pad, buffer() + pad, stream.pos() - pad) != 0)
        return false;
    return true;
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
                             .arg(Mandala::meta(_pid.uid).name)
                             .arg(dump(stream_pos_s));
        emit timeout();
    }
    finish();
}
