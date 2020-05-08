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
#include <Xbus/XbusVehicle.h>

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
    } else if (data.size() > 2) {
        ProtocolTrace::trace(uplink,
                             ProtocolTraceItem::DATA,
                             QString("[%1]%2").arg(data.size()).arg(QString(data.toHex().toUpper())));
    } else {
        ProtocolTrace::trace(uplink,
                             ProtocolTraceItem::DATA,
                             QString("%1").arg(QString(data.toHex().toUpper())));
    }
}
void ProtocolBase::trace(bool uplink, const xbus::pid_s &pid)
{
    if (mandala::cmd::env::nmt::match(pid.uid)) {
        QString s = Mandala::meta(pid.uid).name;
        ProtocolTrace::trace(uplink, ProtocolTraceItem::NMT, s);
    } else {
        QString s = Mandala::meta(pid.uid).path;
        ProtocolTrace::trace(uplink, ProtocolTraceItem::PID, s);
    }
    QString s;
    switch (pid.pri) {
    case xbus::pri_primary:
        s = "pri ";
        break;
    case xbus::pri_secondary:
        s = "sec ";
        break;
    case xbus::pri_failsafe:
        s = "fsf ";
        break;
    case xbus::pri_response:
        s = "R";
        break;
    case xbus::pri_request:
        s = "Q";
        break;
    default:
        s = QString::number(static_cast<int>(pid.pri));
    }
    s = QString("%1%2").arg(s).arg(QString::number(static_cast<int>(pid.seq)));
    ProtocolTrace::trace(uplink, ProtocolTraceItem::DATA, s);
}

void ProtocolBase::trace_downlink(ProtocolTraceItem::TraceType type, const QString &text)
{
    ProtocolTrace::trace(false, type, text);
}
void ProtocolBase::trace_downlink(const QByteArray &data)
{
    trace(false, data);
}
void ProtocolBase::trace_downlink(const xbus::pid_s &pid)
{
    trace(false, pid);
}
void ProtocolBase::trace_downlink(const QString &text)
{
    trace_downlink(ProtocolTraceItem::DATA, text);
}

void ProtocolBase::trace_uplink(ProtocolTraceItem::TraceType type, const QString &text)
{
    ProtocolTrace::trace(true, type, text.contains('.') ? text : text.toUpper());
}
void ProtocolBase::trace_uplink_packet(const QByteArray &data)
{
    trace_uplink(ProtocolTraceItem::PACKET, QString::number(data.size()));

    ProtocolStreamReader stream(data);
    xbus::pid_s pid;
    pid.read(&stream);

    trace(true, pid);

    if (mandala::cmd::env::nmt::match(pid.uid)) {
        if (stream.available() >= sizeof(xbus::node::guid_t)) {
            trace_uplink(ProtocolTraceItem::GUID, "GUID");
            stream.reset(stream.pos() + sizeof(xbus::node::guid_t));
        }
    }
    if (mandala::cmd::env::vehicle::match(pid.uid)) {
        if (stream.available() >= sizeof(xbus::vehicle::squawk_t)) {
            const xbus::vehicle::squawk_t squawk = stream.read<xbus::vehicle::squawk_t>();
            trace_uplink(ProtocolTraceItem::SQUAWK, QString::number(squawk, 16));
            if (stream.available() >= xbus::pid_s::psize()) {
                xbus::pid_s vpid;
                vpid.read(&stream);
                trace(true, vpid);
            }
        }
    }
    trace(true, stream.payload());
}
