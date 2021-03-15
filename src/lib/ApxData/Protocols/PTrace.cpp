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
#include "PTrace.h"

PTrace::PTrace(QObject *parent)
    : QObject(parent)
{}

void PTrace::trace_enable(bool v)
{
    _trace_enabled = v;
    trace_reset();
}

void PTrace::trace_uplink(size_t sz)
{
    if (!_trace_enabled)
        return;

    if (!_blocks.isEmpty())
        trace_end();

    _blocks.append(sz ? QString(">%1").arg(sz) : ">");
}

void PTrace::trace_downlink(size_t sz)
{
    if (!_trace_enabled)
        return;

    if (!_blocks.isEmpty())
        trace_end();

    _blocks.append(sz ? QString("<%1").arg(sz) : "<");
}

void PTrace::trace_end()
{
    if (!_trace_enabled)
        return;

    if (_blocks.isEmpty())
        return;

    qDebug() << _blocks;
    emit trace_blocks(_blocks);
    trace_reset();
}

void PTrace::trace_reset()
{
    _blocks.clear();
}

void PTrace::trace_block(QString block)
{
    if (!_trace_enabled)
        return;

    if (_blocks.isEmpty()) {
        //qWarning() << "orphan trace" << block;
        return;
    }
    _blocks.append(block);
}

void PTrace::trace_data(QByteArray data)
{
    if (!_trace_enabled)
        return;

    if (data.isEmpty())
        return;

    QString s;

    if (data.size() > 16) {
        s = QString("[%1]%2...").arg(data.size()).arg(QString(data.left(16).toHex().toUpper()));
    } else if (data.size() > 2) {
        s = QString("[%1]%2").arg(data.size()).arg(QString(data.toHex().toUpper()));
    } else {
        s = QString("%1").arg(QString(data.toHex().toUpper()));
    }

    trace_block(s);
}
