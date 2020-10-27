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
#include "ProtocolTrace.h"

ProtocolTrace *ProtocolTrace::_instance = nullptr;

ProtocolTrace::ProtocolTrace(QObject *parent)
    : QObject(parent)
{
    _instance = this;
    flushTimer.setSingleShot(true);
    flushTimer.setInterval(10);
    connect(&flushTimer, &QTimer::timeout, this, &ProtocolTrace::flush);
}

void ProtocolTrace::trace(bool uplink, ProtocolTraceItem::TraceType type, const QString &text)
{
    _instance->_trace(uplink, type, text);
}

void ProtocolTrace::_trace(bool uplink, ProtocolTraceItem::TraceType type, const QString &text)
{
    if (type == ProtocolTraceItem::PACKET) {
        flush();
    }
    if (m_uplink != uplink) {
        flush();
        m_uplink = uplink;
    }

    ProtocolTraceItem item;
    item.m_type = type;
    item.m_text = text;
    m_packet.append(QVariant::fromValue(item));

    flushTimer.start();
}

void ProtocolTrace::flush()
{
    flushTimer.stop();
    if (m_packet.isEmpty())
        return;
    /*if (m_uplink) {
        //reverse order
        QVariantList result;
        result.reserve(m_packet.size()); // reserve is new in Qt 4.7
        std::reverse_copy(m_packet.begin(), m_packet.end(), std::back_inserter(result));
        m_packet.swap(result);
    }*/
    emit packet(m_uplink, m_packet);
    m_packet.clear();
}
