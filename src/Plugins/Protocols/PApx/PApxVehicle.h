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

#include "PApx.h"

class PApx;

class PApxVehicle : public PVehicle
{
    Q_OBJECT

public:
    explicit PApxVehicle(PApx *parent,
                         QString callsign,
                         QString uid,
                         VehicleType type,
                         xbus::vehicle::squawk_t squawk);

    void process_downlink(PStreamReader &stream);

    void send_uplink(QByteArray packet) override;

    xbus::vehicle::squawk_t squawk() const { return m_squawk; }
    void setSquawk(xbus::vehicle::squawk_t squawk) { m_squawk = squawk; }

private:
    PApx *_papx;

    xbus::vehicle::squawk_t m_squawk;

    PApxRequest _req;
};
