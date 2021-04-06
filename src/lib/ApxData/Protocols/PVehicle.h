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

#include "PData.h"
#include "PMission.h"
#include "PNodes.h"
#include "PTelemetry.h"

class PBase;

class PVehicle : public PTreeBase
{
    Q_OBJECT

    Q_PROPERTY(QString uid READ uid CONSTANT)

    Q_ENUMS(VehicleType)
    Q_PROPERTY(VehicleType vehicleType READ vehicleType NOTIFY vehicleTypeChanged)
    Q_PROPERTY(QString vehicleTypeText READ vehicleTypeText NOTIFY vehicleTypeChanged)

    Q_ENUMS(StreamType)
    Q_PROPERTY(StreamType streamType READ streamType NOTIFY streamTypeChanged)

    Q_PROPERTY(uint errcnt READ errcnt WRITE setErrcnt NOTIFY errcntChanged)

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

    explicit PVehicle(PBase *parent, QString callsign, QString uid, VehicleType type);

    QString uid() const { return m_uid; }

    VehicleType vehicleType(void) const { return m_vehicleType; }
    void setVehicleType(VehicleType v);
    QString vehicleTypeText() const
    {
        return QMetaEnum::fromType<VehicleType>().valueToKey(vehicleType());
    }

    StreamType streamType(void) const { return m_streamType; }
    void setStreamType(StreamType v); // called by signals of underlying protocols

    uint errcnt(void) const { return m_errcnt; }
    void setErrcnt(const uint &v);
    void incErrcnt();

    // interface to vehicle nodes with parameters
    PNodes *nodes() const { return m_nodes; }

    // interface to C2
    PData *data() const { return m_data; }

    // interface to telemetry downstream
    PTelemetry *telemetry() const { return m_telemetry; }

    // interface to telemetry downstream
    PMission *mission() const { return m_mission; }

protected:
    PNodes *m_nodes{};
    PData *m_data{};
    PTelemetry *m_telemetry{};
    PMission *m_mission{};

private:
    QString m_uid;
    VehicleType m_vehicleType{};
    StreamType m_streamType{};

    QTimer onlineTimer;
    QElapsedTimer time_telemetry;
    QElapsedTimer time_xpdr;

    uint m_errcnt{0};

    void updateValue();

signals:
    void vehicleTypeChanged();
    void streamTypeChanged();
    void errcntChanged();

    // PVehicle interface
signals:
    void packetReceived(mandala::uid_t uid); // used for counters only
};

Q_DECLARE_METATYPE(PVehicle *)
