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

#include "TelemetryFileFormat.h"

class Vehicle;
class XbusStreamWriter;

class TelemetryFile : private QFile
{
    Q_OBJECT

public:
    explicit TelemetryFile();

    bool create(Vehicle *vehicle);
    void write_timestamp(quint32 timestamp_ms);
    void write_values(quint32 timestamp_ms, const PBase::Values &values, bool uplink);

    void write_evt(quint32 timestamp_ms,
                   const QString &name,
                   const QString &value,
                   const QString &uid,
                   bool uplink);
    void write_msg(quint32 timestamp_ms, const QString &text, const QString &subsystem);
    void write_json(const QString &name, const QJsonObject &json);
    void write_raw(quint32 timestamp_ms, uint16_t id, const QByteArray &data, bool uplink);

    void print_stats();

private:
    static constexpr auto suffix = "telemetry";
    static constexpr auto version = 1;

    Vehicle *_vehicle;

    // helpers
    bool _write_tag(XbusStreamWriter *stream, const char *name, const char *value);
    void _write_string(const char *s);
    void _write_uplink();

    void _write_field(QString name, QString title, QString units);

    std::map<mandala::uid_t, uint16_t> _fields_map;
    std::map<mandala::uid_t, QVariant> _values_s;

    std::map<mandala::uid_t, QSet<telemetry::dspec_e>> _stats_values;
    std::map<QString, QJsonObject> _stats_json;

    quint32 _ts_s{};
    uint16_t _widx{};

    void _write_value(mandala::uid_t uid, const QVariant &value, bool uplink);

    void _json_diff(const QJsonObject &prev, const QJsonObject &next, QJsonObject &diff);
};
