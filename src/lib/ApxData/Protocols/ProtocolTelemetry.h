/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#pragma once

#include "ProtocolBase.h"

#include <QtCore>

#include <Xbus/telemetry/TelemetryDecoder.h>
#include <Xbus/uart/CobsDecoder.h>

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

private:
    ProtocolVehicle *vehicle;

    float _rate_s{0};
    uint16_t _ts_s{0};
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
        if (stream.available() != sizeof(T))
            return QVariant();
        return QVariant::fromValue(stream.read<T>());
    }

    uint8_t txbuf[32];
    ProtocolStreamWriter ostream{txbuf, sizeof(txbuf)};
    void pack(const QVariant &v, mandala::type_id_e type, ProtocolStreamWriter &stream);

private slots:
    void updateStatus();

signals:
    void telemetryData(ProtocolTelemetry::TelemetryValues values, quint64 timestamp_ms);
    void valuesData(ProtocolTelemetry::TelemetryValues values);

public slots:
    void sendValue(ProtocolTelemetry::TelemetryValue value);
    void sendBundle(ProtocolTelemetry::TelemetryValues values);
};
