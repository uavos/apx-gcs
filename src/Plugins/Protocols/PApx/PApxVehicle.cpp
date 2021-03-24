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
#include "PApxVehicle.h"

#include "PApxData.h"
#include "PApxTelemetry.h"

PApxVehicle::PApxVehicle(
    PApx *parent, QString callsign, QString uid, VehicleType type, xbus::vehicle::squawk_t squawk)
    : PVehicle(parent, callsign, uid, type)
    , _papx(parent)
    , m_squawk(squawk)
    , _req(parent)
{
    m_data = new PApxData(this);
    m_telemetry = new PApxTelemetry(this);
    //m_nodes = new PApxNodes(this);
}

void PApxVehicle::process_downlink(PStreamReader &stream)
{
    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "packet" << stream.dump_payload();
        return;
    }
    xbus::pid_s pid;
    pid.read(&stream);

    _papx->trace_pid(pid);

    mandala::uid_t uid = pid.uid;

    if (uid > mandala::uid_max) {
        qWarning() << "wrong uid" << uid << stream.dump_payload();
        return;
    }
    emit packetReceived(uid);
}

void PApxVehicle::send_uplink(QByteArray packet)
{
    if (!m_squawk) {
        //local vehicle
        _papx->send_uplink(packet);
        return;
    }

    if (!_papx->addressed(m_squawk)) {
        qWarning() << "not addressed" << PApx::squawkText(m_squawk);
        return;
    }

    _req.request(mandala::cmd::env::vehicle::uplink::uid);

    _req.write<xbus::vehicle::squawk_t>(m_squawk);
    _req.append(packet);

    _req.send();
}
