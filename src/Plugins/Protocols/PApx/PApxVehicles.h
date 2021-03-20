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

#include <xbus/XbusVehicle.h>

#include "PApx.h"
#include "PApxRequest.h"
#include "PApxVehicle.h"

class PApx;
class PApxVehicle;

class PApxVehicles : public PVehicles
{
    Q_OBJECT

public:
    explicit PApxVehicles(PApx *parent);

    bool addressed(xbus::vehicle::squawk_t squawk) const { return _squawk_map.contains(squawk); }

private:
    PApx *_papx;

    PApxVehicle *m_local{};
    QMap<xbus::vehicle::squawk_t, PApxVehicle *> _squawk_map; // identified vehicles
    QList<xbus::vehicle::squawk_t> _squawk_blacklist;

    PApxRequest _req;
    QTimer _reqTimer;
    QList<xbus::vehicle::squawk_t> _req_ident;

    void assign_squawk(const xbus::vehicle::ident_s &ident, QString callsign);
    void request_ident(xbus::vehicle::squawk_t squawk);

    void request_ident_schedule(xbus::vehicle::squawk_t squawk);
    void request_next(); // delayed requests callback

    void process_downlink(QByteArray packet) override;
};
