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

#include <Mandala/Mandala.h>
#include <Mandala/MandalaContainers.h>

class Vehicle;
class XbusStreamWriter;

class TelemetryFile : private QFile
{
public:
    explicit TelemetryFile();

    bool create(Vehicle *vehicle);
    void write_timestamp(quint32 timestamp_ms);
    void write_values(const PBase::Values &values, bool uplink);

private:
    static constexpr auto suffix = "telemetry";
    static constexpr auto version = 1;

    Vehicle *_vehicle;

    // helpers
    bool _write_tag(XbusStreamWriter *stream, const char *name, const char *value);
    void _write_string(const char *s);

    void _write_field(QString name, QString title, QString units);

    std::map<mandala::uid_t, uint16_t> _fields_map;
    std::map<mandala::uid_t, QVariant> _values_s;

    quint32 _ts_s{};
    uint16_t _widx{};

    void _write_value(mandala::uid_t uid, const QVariant &value, bool uplink);
};
