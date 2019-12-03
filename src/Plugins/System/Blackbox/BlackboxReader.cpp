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

#include <Xbus/uart/Escaped.h>
//=============================================================================
BlackboxReader::BlackboxReader(Fact *parent, QString callsign, QString uid)
    : Fact(parent, "reader")
    , Escaped()
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
uint BlackboxReader::esc_read(uint8_t *buf, uint sz)
{
    if (esc_input.isEmpty())
        return 0;
    uint cnt = static_cast<uint>(esc_input.size());
    if (cnt > sz)
        cnt = sz;
    memcpy(buf, esc_input.data(), cnt);
    esc_input.remove(0, static_cast<int>(cnt));
    dataCnt += cnt;
    return cnt;
}
void BlackboxReader::escError(void)
{
    qWarning() << "stream read error";
    if (vehicle->f_telemetry->f_recorder->value().toBool())
        apxMsgW() << tr("Blackbox stream corrupted") << dataCnt;
}
//=============================================================================
void BlackboxReader::processData(QByteArray data)
{
    //qDebug() << data.toHex().toUpper();
    /*if (data != QByteArray(data.size(), (char) 0xFF)) {
        //qDebug() << data.toHex().toUpper();
    } else
        qDebug() << "erased" << req_blk;*/
    esc_input.append(data);
    uint cnt;
    while (!esc_input.isEmpty()) {
        while ((cnt = readEscaped()) > 0) {
            QByteArray packet = QByteArray(reinterpret_cast<const char *>(esc_rx),
                                           static_cast<int>(cnt));

            vehicle->f_telemetry->f_recorder->setValue(true);
            protocol->unpack(packet);
        }
    }
}
//=============================================================================
