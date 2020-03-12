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
#include "ProtocolViewBase.h"

ProtocolViewBaseImpl::ProtocolViewBaseImpl(QObject *parent, Fact *protocol)
    : Fact(parent, "item#", "", QString(), Fact::Group)
{
    setEnabled(false);
    setProtocol(protocol);
}

void ProtocolViewBaseImpl::setProtocol(Fact *protocol)
{
    if (m_protocol == protocol)
        return;

    if (m_protocol) {
        disconnect(m_protocol, nullptr, this, nullptr);
        disconnect(this, nullptr, m_protocol, nullptr);
    }
    m_protocol = protocol;

    if (protocol) {
        connect(protocol, &Fact::destroyed, this, &Fact::remove);

        connect(protocol, &Fact::nameChanged, this, &ProtocolViewBaseImpl::updateName);
        connect(protocol, &Fact::titleChanged, this, &ProtocolViewBaseImpl::updateTitle);
        connect(protocol, &Fact::progressChanged, this, &ProtocolViewBaseImpl::updateProgress);

        updateName();
        updateTitle();
        updateProgress();

        if (icon().isEmpty())
            setIcon(protocol->icon());
    }
    emit protocolChanged();
    setEnabled(protocol);
}

Fact *ProtocolViewBaseImpl::protocol_fact() const
{
    return m_protocol;
}

void ProtocolViewBaseImpl::updateName()
{
    setName(m_protocol->name());
}
void ProtocolViewBaseImpl::updateTitle()
{
    setTitle(m_protocol->title());
}
void ProtocolViewBaseImpl::updateProgress()
{
    setProgress(m_protocol->progress());
}
