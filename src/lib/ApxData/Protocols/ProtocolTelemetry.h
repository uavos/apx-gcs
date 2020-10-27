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

#include "ProtocolBase.h"

#include <QtCore>

#include <xbus/telemetry/TelemetryDecoder.h>
#include <xbus/uart/CobsDecoder.h>

class ProtocolVehicle;

class ProtocolTelemetry : public ProtocolBase
{
    Q_OBJECT

public:
    explicit ProtocolTelemetry(ProtocolVehicle *vehicle);

    // called by nodes
    void downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream);

    struct TelemetryValue
    {
        xbus::pid_s pid;
        QVariant value;
    };
    typedef QList<TelemetryValue> TelemetryValues;
    typedef QList<xbus::pid_s> TelemetryFormat;

private:
    ProtocolVehicle *vehicle;

    uint32_t _seq_s{0};
    uint32_t _seq{0};
    uint32_t _dt_ms{0};
    quint64 _timestamp_ms{0};
    QElapsedTimer _ts_time;

    TelemetryDecoder decoder;

    //CobsDecoder<8192> cobs;

    void request_format(uint8_t part);
    uint8_t _request_format_part{0};
    QElapsedTimer _request_format_time;

    QVariant raw_value(const void *src, mandala::type_id_e type);

    TelemetryValues unpack(const xbus::pid_s &pid,
                           const mandala::spec_s &spec,
                           ProtocolStreamReader &stream);

    template<typename T>
    QVariant unpack(ProtocolStreamReader &stream)
    {
        if (stream.available() < sizeof(T))
            return QVariant();
        return QVariant::fromValue(stream.read<T>());
    }

    uint8_t txbuf[32];
    ProtocolStreamWriter ostream{txbuf, sizeof(txbuf)};
    void pack(const QVariant &v, mandala::type_id_e type, ProtocolStreamWriter &stream);
    void send();

private slots:
    void updateStatus();

signals:
    void formatUpdated(TelemetryFormat format);
    void telemetryData(ProtocolTelemetry::TelemetryValues values, quint64 timestamp_ms);
    void valuesData(ProtocolTelemetry::TelemetryValues values);

public slots:
    void sendValue(mandala::uid_t uid, QVariant value);
};
