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
#include "BlackboxReader.h"

#include <App/AppLog.h>

#include <Fleet/Fleet.h>
#include <Fleet/Unit.h>
#include <Protocols/ProtocolUnit.h>
#include <Telemetry/Telemetry.h>
#include <Telemetry/TelemetryRecorder.h>

BlackboxReader::BlackboxReader(Fact *parent, QString callsign, QString uid)
    : Fact(parent, "reader")
    , protocol(nullptr)
    , unit(nullptr)
    , dataCnt(0)
{
    //setEnabled(false);
    //create tmp unit
    ProtocolFleet::IdentData ident;
    ident.uid = uid;
    ident.vclass = Unit::TEMPORARY;
    QStringList st;
    QString s = callsign;
    if (!s.isEmpty())
        st << s;
    st << "BLACKBOX";
    ident.callsign = st.join('-');

    protocol = new ProtocolUnit(0, ident, nullptr);
    unit = Fleet::instance()->createUnit(protocol);
    unit->setParentFact(this);
    bind(unit);
}

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
        unit->f_telemetry->f_recorder->setRecording(true);
        protocol->downlinkData(packet.left(static_cast<int>(cnt)));
    }
}
