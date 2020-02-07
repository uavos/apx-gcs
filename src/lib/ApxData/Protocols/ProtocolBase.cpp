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
#include "ProtocolConverter.h"

ProtocolBase::ProtocolBase(QObject *parent)
    : QObject(parent)
    , m_converter(nullptr)
    , m_progress(-1)
{
    reqTimer.setInterval(500);
    connect(&reqTimer, &QTimer::timeout, this, [=]() {
        if (reqList.isEmpty())
            reqTimer.stop();
        else
            send(reqList.takeFirst());
    });
}

void ProtocolBase::setConverter(ProtocolConverter *c)
{
    if (m_converter) {
        disconnect(m_converter, nullptr, this, nullptr);
    }
    m_converter = c;
    if (m_converter) {
        connect(m_converter, &ProtocolConverter::convertUplink, this, &ProtocolBase::uplinkData);
        connect(m_converter, &ProtocolConverter::downlink, this, &ProtocolBase::unpack);
    }
}

void ProtocolBase::downlinkData(QByteArray packet)
{
    if (m_converter)
        m_converter->convertDownlink(packet);
    else
        unpack(packet);
}
void ProtocolBase::send(QByteArray packet)
{
    if (m_converter)
        m_converter->convertUplink(packet);
    else
        emit uplinkData(packet);
}

void ProtocolBase::unpack(const QByteArray packet)
{
    Q_UNUSED(packet)
}

void ProtocolBase::scheduleRequest(QByteArray packet)
{
    if (!reqList.contains(packet)) {
        reqList.append(packet);
        reqTimer.start();
    }
}

int ProtocolBase::progress() const
{
    return m_progress;
}
void ProtocolBase::setProgress(int v)
{
    if (m_progress == v)
        return;
    m_progress = v;
    emit progressChanged();
}
QString ProtocolBase::status() const
{
    return m_status;
}
void ProtocolBase::setStatus(const QString &v)
{
    if (m_status == v)
        return;
    m_status = v;
    emit statusChanged();
}
