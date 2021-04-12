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
#include "PNode.h"

#include "PFirmware.h"
#include "PVehicle.h"

PNode::PNode(PNodes *parent, QString uid)
    : PTreeBase(parent, "node#", "node", "", Group)
    , m_uid(uid)
{
    auto p = findParent<PBase>()->firmware();
    if (p) {
        connect(p, &PFirmware::upgradeStarted, this, &PNode::upgradeStarted);
        connect(p, &PFirmware::upgradeFinished, this, &PNode::upgradeFinished);

        _upgradingDoneTimer.setSingleShot(true);
        _upgradingDoneTimer.setInterval(1000);
        connect(&_upgradingDoneTimer, &QTimer::timeout, this, [this]() { setUpgrading(false); });
    } else {
        qWarning() << "missing PApxFirmware";
    }
}

void PNode::setUpgrading(bool v)
{
    _upgradingDoneTimer.stop();
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    setActive(v);
    emit upgradingChanged();
}

void PNode::upgradeStarted(QString uid, QString name)
{
    if (uid != m_uid)
        return;
    setUpgrading(true);

    auto p = findParent<PBase>()->firmware();
    bindProperty(p, "value", true);
    bindProperty(p, "progress", true);
}
void PNode::upgradeFinished(QString uid, bool success)
{
    //qDebug() << uid << success;
    if (uid != m_uid)
        return;
    // _upgradingDoneTimer.start();

    auto p = findParent<PBase>()->firmware();
    unbindProperties(p);
    setValue(QVariant());
    setProgress(-1);

    setUpgrading(false);
}
