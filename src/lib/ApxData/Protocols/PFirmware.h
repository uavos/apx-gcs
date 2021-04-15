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

#include "PTrace.h"
#include "PTreeBase.h"

class PBase;

class PFirmware : public PTreeBase
{
    Q_OBJECT

    Q_PROPERTY(bool upgrading READ upgrading NOTIFY upgradingChanged)

public:
    explicit PFirmware(PBase *parent);

    auto upgrading() const { return m_upgrading; }
    void setUpgrading(bool v);

    virtual void upgradeFirmware(QString uid, QString name, QByteArray data, quint32 offset);

private:
    bool m_upgrading{};

protected:
    QString _uid;
    QString _name;
    QByteArray _data;
    quint32 _offset;

    bool _success{};

protected slots:
    virtual void finish();

signals:
    void upgradingChanged();
    void upgradeStarted(QString uid, QString name);
    void upgradeFinished(QString uid, bool success);
};
