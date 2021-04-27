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
#include "PFirmware.h"

#include "PBase.h"

PFirmware::PFirmware(PBase *parent)
    : PTreeBase(parent, "firmware", tr("Firmware"), tr("Node upgrade interface"))
{
    connect(this, &PFirmware::upgradeFinished, this, [this]() { setUpgrading(false); });
    connect(root(), &PBase::cancelRequests, this, &PFirmware::finish, Qt::QueuedConnection);
}

void PFirmware::setUpgrading(bool v)
{
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    setActive(v);
    emit upgradingChanged();
    if (!v)
        setProgress(-1);
}

void PFirmware::upgradeFirmware(QString uid, QString name, QByteArray data, quint32 offset)
{
    _success = false;

    _uid = uid;
    _name = name;
    _data = data;
    _offset = offset;

    setProgress(0);
    setUpgrading(true);
    emit upgradeStarted(uid, name);

    setValue(tr("Initializing..."));
}

void PFirmware::finish()
{
    qDebug() << _name << _uid;

    emit upgradeFinished(_uid, _success);

    setValue(QVariant());
    _success = false;
}
