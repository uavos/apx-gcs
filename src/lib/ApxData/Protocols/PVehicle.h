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

#include "PVehicles.h"

class PNodes;
class PMission;
class PData;
class PTelemetry;

class PVehicles;

class PVehicle : public PTreeBase
{
    Q_OBJECT

    Q_PROPERTY(PNodes nodes READ nodes CONSTANT)
    Q_PROPERTY(PMission mission READ mission CONSTANT)
    Q_PROPERTY(PData data READ data CONSTANT)
    Q_PROPERTY(PTelemetry telemetry READ telemetry CONSTANT)

    Q_PROPERTY(QString uid READ uid CONSTANT)

    Q_ENUMS(VehicleType)
    Q_PROPERTY(VehicleType vehicleType READ vehicleType NOTIFY vehicleTypeChanged)
    Q_PROPERTY(QString vehicleTypeText READ vehicleTypeText NOTIFY vehicleTypeChanged)

    Q_ENUMS(StreamType)
    Q_PROPERTY(StreamType streamType READ streamType NOTIFY streamTypeChanged)
    Q_PROPERTY(QString streamTypeText READ streamTypeText NOTIFY streamTypeChanged)

public:
    enum VehicleType {
        UAV = 0, // unmanned vehicle
        GCS,     // Ground Control
    };
    Q_ENUM(VehicleType)

    enum StreamType {
        OFFLINE = 0, // no data received
        NMT,         // network management packets only
        DATA,        // data packets from nodes with some mandala parameters
        XPDR,        // low-bandwidth limited telemetry data stream and C2
        TELEMETRY    // full telemetry data stream is available
    };
    Q_ENUM(StreamType)

    explicit PVehicle(PVehicles *parent, QString callsign, QString uid, VehicleType type);

    QString uid() const { return name(); }

    VehicleType vehicleType(void) const { return m_vehicleType; }
    void setVehicleType(VehicleType v);
    QString vehicleTypeText() const
    {
        return QMetaEnum::fromType<VehicleType>().valueToKey(vehicleType());
    }

    StreamType streamType(void) const { return m_streamType; }
    QString streamTypeText() const
    {
        return QMetaEnum::fromType<StreamType>().valueToKey(streamType());
    }

    // interface to vehicle nodes with parameters
    PNodes *nodes() const { return m_nodes; }

    // interface to vehicle mission
    PMission *mission() const { return m_mission; }

    // interface to C2
    PData *data() const { return m_data; }

    // interface to telemetry downstream
    PTelemetry *telemetry() const { return m_telemetry; }

protected:
    void setStreamType(StreamType v);

private:
    PNodes *m_nodes{};
    PMission *m_mission{};
    PTelemetry *m_telemetry{};
    PData *m_data{};

    VehicleType m_vehicleType{};
    StreamType m_streamType{};

    QTimer onlineTimer;
    QElapsedTimer time_telemetry;
    QElapsedTimer time_xpdr;

signals:
    void vehicleTypeChanged();
    void streamTypeChanged();
};
