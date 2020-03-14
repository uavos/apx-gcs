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
    virtual void setTitle(const QString &v);
    virtual void setDescr(const QString &v);
    virtual bool setValue(const QVariant &v);

private slots:
    void updateName();
    void updateTitle();
    void updateDescr();
    void updateValue();
    void updateProgress();

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
