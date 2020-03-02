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
#include "BlackboxReader.h"

#include <App/AppLog.h>

#include <Protocols/ProtocolVehicle.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryRecorder.h>
#include <Vehicles/Vehicle.h>
#include <Vehicles/Vehicles.h>
//=============================================================================
BlackboxReader::BlackboxReader(Fact *parent, QString callsign, QString uid)
    : Fact(parent, "reader")
    , protocol(nullptr)
    , vehicle(nullptr)
    , dataCnt(0)
{
    //setEnabled(false);
    //create tmp vehicle
    ProtocolVehicles::IdentData ident;
    ident.uid = uid;
    ident.vclass = Vehicle::TEMPORARY;
    QStringList st;
    QString s = callsign;
    if (!s.isEmpty())
        st << s;
    st << "BLACKBOX";
    ident.callsign = st.join('-');

    protocol = new ProtocolVehicle(0, ident, nullptr);
    vehicle = Vehicles::instance()->createVehicle(protocol);
    vehicle->setParentFact(this);
    bind(vehicle);
}
//=============================================================================
void BlackboxReader::processData(QByteArray data)
{
    //qDebug() << data.toHex().toUpper();
    /*if (data != QByteArray(data.size(), (char) 0xFF)) {
        //qDebug() << data.toHex().toUpper();
    } else
        qDebug() << "erased" << req_blk;*/
    SerialDecoder::ErrorType rv = esc_reader.decode(data.data(), static_cast<size_t>(data.size()));
    switch (rv) {
    case SerialDecoder::DataAccepted:
    case SerialDecoder::DataDropped:
        break;
    default:
        apxConsoleW() << "SerialDecoder rx ovf:" << data.size() << esc_reader.size();
    }
    QByteArray packet(static_cast<int>(esc_reader.size()), '\0');
    while (esc_reader.size() > 0) {
        size_t cnt = esc_reader.read_packet(packet.data(), static_cast<size_t>(packet.size()));
        if (!cnt)
            break;
        vehicle->f_telemetry->f_recorder->setRecording(true);
        protocol->downlinkData(packet.left(static_cast<int>(cnt)));
    }
}
//=============================================================================
