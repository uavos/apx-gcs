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
#include "ProtocolTelemetry.h"
#include "ProtocolVehicle.h"

#include <App/App.h>
#include <Mandala/Mandala.h>

ProtocolTelemetry::ProtocolTelemetry(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle, "telemetry")
    , vehicle(vehicle)
{
    setIcon("sitemap");
    setTitle(tr("Telemetry"));
    setDescr(tr("Downlink stream decoder"));

    connect(this, &Fact::enabledChanged, this, &ProtocolTelemetry::updateStatus);
}

void ProtocolTelemetry::downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    trace_downlink(stream.payload());

    if (stream.available() < xbus::telemetry::stream_s::psize()) {
        qWarning() << stream.available();
        return;
    }

    bool valid = decode(pid, stream);
    setEnabled(valid);

    if (valid && _rate_s != _rate) {
        _rate_s = _rate;
        updateStatus();
    }

    qDebug() << valid;
    //qDebug() << seq << _fmt_decoder.decode(&hdr.feed_fmt, 1) << _fmt_decoder.size();
}

void ProtocolTelemetry::updateStatus()
{
    if (!enabled()) {
        setValue("RESYNC");
        return;
    }
    setValue(QString("%1 slots, %2 Hz").arg(_slots_cnt).arg(static_cast<int>(rate_hz())));
}
