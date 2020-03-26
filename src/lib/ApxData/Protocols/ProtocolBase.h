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
#pragma once

#include <QtCore>

#include <Fact/Fact.h>

#include "ProtocolStream.h"
#include "ProtocolTrace.h"
#include <Mandala/Mandala.h>

class ProtocolBase : public Fact
{
    Q_OBJECT
public:
    explicit ProtocolBase(QObject *parent, const QString &name);

    void trace(bool uplink, const QByteArray &data);
    void trace(bool uplink, const xbus::pid_s &pid);

    void trace_downlink(ProtocolTraceItem::TraceType type, const QString &text);
    void trace_downlink(const QByteArray &data);
    void trace_downlink(const xbus::pid_s &pid);
    void trace_downlink(const QString &text);

    void trace_uplink(ProtocolTraceItem::TraceType type, const QString &text);
    void trace_uplink_packet(const QByteArray &data);
};
