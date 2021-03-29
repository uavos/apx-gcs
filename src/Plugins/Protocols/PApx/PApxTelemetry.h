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

#include "PApxVehicle.h"

#include <xbus/telemetry/TelemetryDecoder.h>
#include <xbus/uart/CobsDecoder.h>

class PApxVehicle;

class PApxTelemetry : public PTelemetry
{
    Q_OBJECT

public:
    explicit PApxTelemetry(PApxVehicle *parent);

    bool process_downlink(const xbus::pid_s &pid, PStreamReader &stream);

private:
    PApxRequest _req;

    uint32_t _seq_s{0};
    uint32_t _seq{0};
    uint32_t _dt_ms{0};
    quint64 _timestamp_ms{0};
    QElapsedTimer _ts_time;

    TelemetryDecoder decoder;

    void request_format(uint8_t part);
    uint8_t _request_format_part{0};
    QElapsedTimer _request_format_time;

    bool unpack(uint8_t pseq, PStreamReader &stream);
    QVariant raw_value(const void *src, mandala::type_id_e type);

    bool unpack_xpdr(PStreamReader &stream);

    void requestTelemetry() override;

private slots:
    void updateStatus();

public slots:
    void report();
};
