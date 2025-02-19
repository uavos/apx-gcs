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
#include "PUnit.h"

PUnit::PUnit(PBase *parent, QString callsign, QString uid, UnitType type)
    : PTreeBase(parent,
                uid.isEmpty() ? callsign.toLower() : "unit#",
                callsign,
                tr("Unit interface"),
                Group | Count)
    , m_uid(uid)
    , m_unitType(type)
{
    qDebug() << "available" << unitTypeText() << callsign << uid;

    onlineTimer.setSingleShot(true);
    onlineTimer.setInterval(7000);
    connect(&onlineTimer, &QTimer::timeout, this, [this]() { setStreamType(OFFLINE); });

    time_telemetry.start();
    time_xpdr.start();

    connect(this, &PUnit::streamTypeChanged, this, &PUnit::updateValue);
    updateValue();
}

void PUnit::setUnitType(UnitType v)
{
    if (m_unitType == v)
        return;
    m_unitType = v;
    emit unitTypeChanged();
}

void PUnit::setStreamType(StreamType type)
{
    if (type != OFFLINE) {
        onlineTimer.start();

        switch (type) {
        default:
            break;
        case TELEMETRY:
            time_telemetry.start();
            break;
        case XPDR:
            time_xpdr.start();
            break;

        case SYS:
        case DATA:
            if (time_telemetry.elapsed() < 2000 || time_xpdr.elapsed() < 3000)
                return;
            break;
        }
    }
    if (m_streamType == type)
        return;

    m_streamType = type;
    emit streamTypeChanged();
}

void PUnit::setErrcnt(const uint &v)
{
    if (m_errcnt == v)
        return;
    m_errcnt = v;
    emit errcntChanged();
    if (!v) {
        qDebug() << "errcnt reset";
    }
}
void PUnit::incErrcnt()
{
    m_errcnt++;
    emit errcntChanged();
}

void PUnit::updateValue()
{
    setValue(QMetaEnum::fromType<StreamType>().valueToKey(streamType()));
}
