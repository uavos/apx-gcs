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
    }
    m_protocol = protocol;

    if (protocol) {
        connect(protocol, &Fact::destroyed, this, &Fact::remove);

        connect(protocol, &Fact::nameChanged, this, &ProtocolViewBaseImpl::updateName);
        connect(protocol, &Fact::titleChanged, this, &ProtocolViewBaseImpl::updateTitle);
        connect(protocol, &Fact::descrChanged, this, &ProtocolViewBaseImpl::updateDescr);
        connect(protocol, &Fact::valueChanged, this, &ProtocolViewBaseImpl::updateValue);
        connect(protocol, &Fact::progressChanged, this, &ProtocolViewBaseImpl::updateProgress);

        bindProperty(protocol, "active");

        updateName();
        updateTitle();
        updateDescr();
        updateValue();
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
    Fact::setTitle(m_protocol->title());
}
void ProtocolViewBaseImpl::updateDescr()
{
    Fact::setDescr(m_protocol->descr());
}
void ProtocolViewBaseImpl::updateValue()
{
    Fact::setValue(m_protocol->value());
}
void ProtocolViewBaseImpl::updateProgress()
{
    setProgress(m_protocol->progress());
}

void ProtocolViewBaseImpl::setTitle(const QString &v)
{
    if (m_protocol)
        disconnect(m_protocol, &Fact::titleChanged, this, &ProtocolViewBaseImpl::updateTitle);
    Fact::setTitle(v);
}
void ProtocolViewBaseImpl::setDescr(const QString &v)
{
    if (m_protocol)
        disconnect(m_protocol, &Fact::descrChanged, this, &ProtocolViewBaseImpl::updateDescr);
    Fact::setDescr(v);
}
bool ProtocolViewBaseImpl::setValue(const QVariant &v)
{
    if (m_protocol)
        disconnect(m_protocol, &Fact::valueChanged, this, &ProtocolViewBaseImpl::updateValue);
    return Fact::setValue(v);
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
