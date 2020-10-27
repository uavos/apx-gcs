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
#include "ProtocolViewBase.h"

ProtocolViewBaseImpl::ProtocolViewBaseImpl(QObject *parent, Fact *protocol)
    : Fact(parent, "item#")
{
    setTreeType(Group);
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
        unbindProperties(m_protocol);
    }
    m_protocol = protocol;

    if (protocol) {
        connect(protocol, &Fact::destroyed, this, &Fact::remove);

        bindProperty(protocol, "name", true);
        bindProperty(protocol, "title", true);
        bindProperty(protocol, "descr", true);
        bindProperty(protocol, "value", false);
        bindProperty(protocol, "progress", true);

        bindProperty(protocol, "active", true);

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

QString ProtocolViewBaseImpl::toolTip() const
{
    if (m_protocol)
        return m_protocol->toolTip();
    else
        return Fact::toolTip();
}
void ProtocolViewBaseImpl::hashData(QCryptographicHash *h) const
{
    if (m_protocol)
        m_protocol->hashData(h);
    else
        Fact::hashData(h);
}
