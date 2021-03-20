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
#include "PApx.h"
#include "PApxVehicles.h"

#include <Mandala/Mandala.h>

PApx::PApx(Fact *parent)
    : PBase(parent, "papx", tr("APX Protocol"), tr("Standard data format for APX Autopilot"))
{
    m_vehicles = new PApxVehicles(this);
}

void PApx::trace_pid(const xbus::pid_s &pid)
{
    if (!trace()->enabled())
        return;

    trace()->block(QString("$%1").arg(Mandala::meta(pid.uid).path));

    QString s;
    switch (pid.pri) {
    case xbus::pri_final:
        s = "F";
        break;
    case xbus::pri_primary:
        s = "P";
        break;
    case xbus::pri_secondary:
        s = "S";
        break;
    case xbus::pri_failsafe:
        s = "E";
        break;
    case xbus::pri_response:
        s = "R";
        break;
    case xbus::pri_request:
        s = "Q";
        break;
    default:
        s = QString::number(static_cast<int>(pid.pri));
    }
    s = QString("%1%2").arg(s).arg(QString::number(static_cast<int>(pid.seq)));
    trace()->block(s);
}
