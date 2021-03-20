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

#include <App/AppLog.h>

APX_LOGGING_CATEGORY(log, "PApx")

PTrace::PTrace(QObject *parent)
    : QObject(parent)
{
    AppLog::add(log().categoryName(),
                QString("Protocol-%1.txt").arg(parent ? parent->objectName() : "any"),
                false);
}

void PTrace::enable(bool v)
{
    _enabled = v;
    reset();
}

void PTrace::uplink()
{
    if (!_enabled)
        return;

    if (!_blocks.isEmpty() && _blocks.first() != ">")
        end();

    _blocks.append(">");
}

void PTrace::downlink(size_t sz)
{
    if (!_enabled)
        return;

    if (!_blocks.isEmpty())
        end();

    _blocks.append(sz ? QString("<%1").arg(sz) : "<");
}

void PTrace::end()
{
    if (!_enabled)
        return;

    if (_blocks.isEmpty())
        return;

    if (_blocks.first() == ">") {
        // re-arrange uplink nested blocks
        for (int i = _blocks.lastIndexOf(">"); i > 0; i = _blocks.lastIndexOf(">")) {
            QStringList tail = _blocks.mid(i);
            tail.append("+");
            tail.append(_blocks.mid(1, i - 1));
            _blocks = tail;
        }
    }

    // qDebug() << _blocks;
    qInfo(&log) << _blocks;
    // QMessageLogger(0, 0, 0).info(&log) << _blocks;

    emit packet(_blocks);
    reset();
}

void PTrace::reset()
{
    _blocks.clear();
}

void PTrace::block(QString block)
{
    if (!_enabled)
        return;

    if (_blocks.isEmpty())
        return;

    _blocks.append(block);
}
void PTrace::blocks(QStringList blocks)
{
    if (!_enabled)
        return;
    if (_blocks.isEmpty())
        return;

    _blocks.append(blocks);
}

void PTrace::data(QByteArray data)
{
    if (!_enabled)
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

    block(s);
}
