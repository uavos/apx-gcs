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

class PApxUnit : public PUnit
{
    Q_OBJECT

public:
    explicit PApxUnit(PApx *parent,
                      QString callsign,
                      UnitType type,
                      const xbus::unit::uid_t *uid_raw,
                      xbus::unit::squawk_t squawk);

    void process_downlink(const xbus::pid_s &pid, PStreamReader &stream);

    void send_uplink(QByteArray packet) override;

    auto squawk() const { return _squawk; }
    void setSquawk(xbus::unit::squawk_t squawk) { _squawk = squawk; }

    static QString uidText(const xbus::unit::uid_t *uid_raw);

    const auto &vuid() const { return _vuid; }
    bool check_vuid(uint8_t n, uint8_t seq) const;

private:
    PApx *_papx;

    xbus::unit::squawk_t _squawk;
    xbus::unit::uid_t _vuid{};

    PApxRequest _req;
};
