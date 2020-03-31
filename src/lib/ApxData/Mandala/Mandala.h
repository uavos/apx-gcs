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

#include "MandalaFact.h"
#include <Fact/Fact.h>
#include <Mandala/MandalaMetaTree.h>

class Mandala : public Fact
{
    Q_OBJECT

public:
    explicit Mandala(Fact *parent = nullptr);

    Q_INVOKABLE MandalaFact *fact(mandala::uid_t uid) const;
    Q_INVOKABLE MandalaFact *fact(const QString &mpath) const;

    QHash<QString, QVariant> constants; // <name,value> enums in form varname_ENUM
    QMap<mandala::uid_t, MandalaFact *> uid_map;

    quint64 timestamp() const;

    static const mandala::meta_s &meta(mandala::uid_t uid);

protected:
    // Fact override
    virtual QString mandalaToString(quint16 uid) const override;
    virtual quint16 stringToMandala(const QString &s) const override;

private:
    quint64 m_timestamp;

public slots:
    void receivedData(xbus::pid_s pid, ProtocolStreamReader *stream);

signals:
    void sendUplink(QByteArray data); //forwarded to vehicle
};
