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

#include <App/AppGcs.h>

#include "PApxUnit.h"

#include "PApxData.h"
#include "PApxMission.h"
#include "PApxNodes.h"
#include "PApxTelemetry.h"

PApxUnit::PApxUnit(PApx *parent,
                   QString callsign,
                   UnitType type,
                   const xbus::unit::uid_t *uid_raw,
                   xbus::unit::squawk_t squawk)
    : PUnit(parent, callsign, uidText(uid_raw), type)
    , _papx(parent)
    , _squawk(squawk)
    , _req(parent)
{
    if (uid_raw)
        memcpy(_vuid, uid_raw, sizeof(xbus::unit::uid_t));

    m_data = new PApxData(this);
    m_telemetry = new PApxTelemetry(this);
    m_nodes = new PApxNodes(this);
    m_mission = new PApxMission(this);

    AppGcs::jsync(this);
}

QString PApxUnit::uidText(const xbus::unit::uid_t *uid_raw)
{
    if (!uid_raw)
        return {};
    return QByteArray(reinterpret_cast<const char *>(uid_raw), sizeof(xbus::unit::uid_t))
        .toHex()
        .toUpper();
}

void PApxUnit::process_downlink(PStreamReader &stream)
{
    if (stream.available() < xbus::pid_s::psize()) {
        qWarning() << "packet" << stream.dump_payload();
        return;
    }
    stream.trim();

    xbus::pid_s pid;
    pid.read(&stream);

    _papx->trace_pid(pid);

    mandala::uid_t uid = pid.uid;

    if (uid > mandala::uid_max) {
        qWarning() << "wrong uid" << uid << stream.dump_payload();
        return;
    }
    emit packetReceived(uid);

    if (static_cast<PApxNodes *>(m_nodes)->process_downlink(pid, stream)) {
        setStreamType(SYS);
        return;
    }

    if (static_cast<PApxData *>(m_data)->process_downlink(pid, stream)) {
        return;
    }

    if (static_cast<PApxTelemetry *>(m_telemetry)->process_downlink(pid, stream)) {
        return;
    }
}

void PApxUnit::send_uplink(QByteArray packet)
{
    if (!_squawk) {
        //local unit
        _papx->send_uplink(packet);
        return;
    }

    if (!_papx->addressed(_squawk)) {
        qWarning() << "not addressed" << PApx::squawkText(_squawk);
        return;
    }

    _req.request(mandala::cmd::env::unit::uplink::uid);

    _req.write<xbus::unit::squawk_t>(_squawk);
    trace()->block(PApx::squawkText(_squawk));
    trace()->block(title().append(':'));
    _req.append(packet);

    _req.send();
}

bool PApxUnit::check_vuid(uint8_t n, uint8_t seq) const
{
    // seq=[0,1,2,3]
    seq &= 3;
    for (uint8_t i = 0; i < sizeof(xbus::unit::uid_t); i += 4)
        if (_vuid[i + seq] == n)
            return true;
    return false;
}
