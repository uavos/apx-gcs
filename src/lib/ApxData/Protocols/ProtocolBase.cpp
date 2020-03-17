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
#include "ProtocolBase.h"
#include <Mandala/Mandala.h>
#include <Xbus/XbusNode.h>

ProtocolBase::ProtocolBase(QObject *parent, const QString &name)
    : Fact(parent, name.toLower(), name, "", ProgressTrack)
{}

void ProtocolBase::trace(bool uplink, const QByteArray &data)
{
    if (data.isEmpty())
        return;

    if (data.size() > 16) {
        ProtocolTrace::trace(uplink,
                             ProtocolTraceItem::DATA,
                             QString("[%1]%2...")
                                 .arg(data.size())
                                 .arg(QString(data.left(16).toHex().toUpper())));
    } else {
        ProtocolTrace::trace(uplink,
                             ProtocolTraceItem::DATA,
                             QString("[%1]%2").arg(data.size()).arg(QString(data.toHex().toUpper())));
    }
}

void ProtocolBase::trace_downlink(ProtocolTraceItem::TraceType type, const QString &text)
{
    ProtocolTrace::trace(false, type, text.contains('.') ? text : text.toUpper());
}
void ProtocolBase::trace_downlink(const QByteArray &data)
{
    trace(false, data);
}

void ProtocolBase::trace_uplink(ProtocolTraceItem::TraceType type, const QString &text)
{
    ProtocolTrace::trace(true, type, text.contains('.') ? text : text.toUpper());
}
void ProtocolBase::trace_uplink_packet(const QByteArray &data)
{
    trace_uplink(ProtocolTraceItem::PACKET, QString::number(data.size()));

    ProtocolStreamReader stream(data);
    xbus::pid_t pid;
    stream >> pid;

    if (mandala::cmd::env::nmt::match(pid)) {
        trace_uplink(ProtocolTraceItem::NMT, Mandala::meta(pid).name);
        if (stream.available() >= sizeof(xbus::node::guid_t)) {
            trace_uplink(ProtocolTraceItem::GUID, "GUID");
            stream.reset(stream.pos() + sizeof(xbus::node::guid_t));
        }
    } else {
        trace_uplink(ProtocolTraceItem::PID, Mandala::meta(pid).path);
    }
    trace(true, stream.payload());
}
