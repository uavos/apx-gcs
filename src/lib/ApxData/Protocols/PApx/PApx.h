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

#include <Protocols/PBase.h>
#include <Protocols/PStream.h>

#include <xbus/XbusVehicle.h>

class PApxVehicle;

class PApx : public PBase
{
    Q_OBJECT

public:
    explicit PApx(QObject *parent = nullptr);

    void trace(const xbus::pid_s &pid);

private:
    QMap<xbus::vehicle::squawk_t, PApxVehicle *> _vehicles;

    uint8_t _txbuf[xbus::size_packet_max];
    PStreamWriter _ostream{_txbuf, sizeof(_txbuf)};

    void process_downlink(QByteArray packet) override;

    void assign_squawk(const xbus::vehicle::ident_s &ident, const QString &callsign);
};
