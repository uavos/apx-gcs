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
#pragma once

#include <QtCore>

#include <Fact/Fact.h>

class ProtocolViewBaseImpl : public Fact
{
    Q_OBJECT
    Q_PROPERTY(Fact *protocol READ protocol_fact NOTIFY protocolChanged)

public:
    explicit ProtocolViewBaseImpl(QObject *parent, Fact *protocol);

    Fact *protocol_fact() const;

private:
    Fact *m_protocol{nullptr};
    void setProtocol(Fact *protocol);

protected:
    virtual QString toolTip() const override;
    virtual void hashData(QCryptographicHash *h) const override;

signals:
    void protocolChanged();
};

template<class _T>
class ProtocolViewBase : public ProtocolViewBaseImpl
{
public:
    explicit ProtocolViewBase(QObject *parent, _T *protocol)
        : ProtocolViewBaseImpl(parent, protocol)
    {}

    _T *protocol() const { return qobject_cast<_T *>(protocol_fact()); }
};
