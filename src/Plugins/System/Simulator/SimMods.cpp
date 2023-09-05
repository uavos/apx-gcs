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

#include "SimMods.h"

#include <tcp_ports.h>

#include <MandalaBundles.h>
#include <MandalaMetaTree.h>

SimMods::SimMods(Fact *parent)
    : Fact(parent, "mods", tr("Jamming"), tr("Alter sensors data"), Group)
{
    f_enb = new Fact(this, "enb", tr("Enable Jamming"), tr("Intercept SIM sensors"), Bool);

    connect(f_enb, &Fact::valueChanged, this, [this]() {
        if (f_enb->value().toBool()) {
            _udp_sim.bind(UDP_PORT_AP_SNS);
        } else {
            _udp_sim.close();
        }
    });

    connect(&_udp_sim, &QUdpSocket::readyRead, this, &SimMods::simRead);

    f_ext_enb = new Fact(this,
                         "ext_enb",
                         tr("Enable External JSON mods"),
                         tr("Modify JSON over UDP"),
                         Bool);

    f_ap_port = new Fact(this,
                         "ap_port",
                         tr("Autopilot port"),
                         tr("UDP port to send modified sensors to"),
                         Int);
    f_ap_port->setMin(0);
    f_ap_port->setMax(65535);
    f_ap_port->setValue(UDP_PORT_AP_SNS + 2);

    f_mod_period = new Fact(this, "mod_period", tr("Period"), tr("Modify period in ms"), Int);
}

void SimMods::simRead()
{
    while (_udp_sim.hasPendingDatagrams()) {
        qint64 sz = _udp_sim.pendingDatagramSize();
        // qDebug() << sz;
        QNetworkDatagram datagram = _udp_sim.receiveDatagram();
        if (!datagram.isValid())
            continue;

        auto rcnt = datagram.data().size();

        const uint8_t *data = (const uint8_t *) datagram.data().constData();
        while (rcnt > 0) {
            auto dcnt = _dec.decode(data, rcnt);

            switch (_dec.status()) {
            case SerialDecoder::PacketAvailable: {
                data = static_cast<const uint8_t *>(data) + dcnt;
                parse_rx(_dec.data(), _dec.size());
                break;
            }

            case SerialDecoder::DataAccepted:
            case SerialDecoder::DataDropped:
                rcnt = 0;
                break;
            default:
                break;
            }

            if (rcnt > dcnt)
                rcnt -= dcnt;
            else
                rcnt = 0;
        }
    }
}

void SimMods::parse_rx(const void *data, size_t size)
{
    XbusStreamReader stream(data, size);
    if (stream.available() < xbus::pid_s::psize())
        return;
    xbus::pid_s pid;
    pid.read(&stream);

    mandala::uid_t uid = pid.uid;
    if (!mandala::cmd::env::sim::match(uid))
        return;

    // qDebug() << "RX" << size << uid;

    switch (uid) {
    default:
        qDebug() << "RX" << size << uid;
        break;
    case mandala::cmd::env::sim::sns::uid:
        if (stream.available() < sizeof(mandala::bundle::sim_s)) {
            qWarning() << "RX" << size << uid << "too small";
            return;
        }

        auto d = (const mandala::bundle::sim_s *) stream.ptr();
        // serialize bundle to variant map
        QVariantMap map;

        map["att_deg"] = QVariantList{d->att_deg[0], d->att_deg[1], d->att_deg[2]};
        map["gyro_deg"] = QVariantList{d->gyro_deg[0], d->gyro_deg[1], d->gyro_deg[2]};
        map["acc_ned"] = QVariantList{d->acc_ned[0], d->acc_ned[1], d->acc_ned[2]};
        map["lat_deg"] = d->lat_deg;
        map["lon_deg"] = d->lon_deg;
        map["hmsl"] = d->hmsl;
        map["vel_ned"] = QVariantList{d->vel_ned[0], d->vel_ned[1], d->vel_ned[2]};
        map["agl"] = d->agl;
        map["rpm"] = d->rpm;
        map["airspeed"] = d->airspeed;
        map["baro_mbar"] = d->baro_mbar;
        map["room_temp"] = d->room_temp;
        map["air_temp"] = d->air_temp;
        map["slip"] = d->slip;

        // qDebug() << map;

        // convert to JSON
        QJsonDocument doc = QJsonDocument::fromVariant(map);
        QByteArray json = doc.toJson(QJsonDocument::Compact);

        qDebug() << json;

        return;
    }

    // forward to APX autopilot app as-is
    auto cnt = _enc.encode(data, size);
    if (cnt <= 0)
        return;
    _udp_sim.writeDatagram(QByteArray((const char *) _enc.data(), cnt),
                           QHostAddress::LocalHost,
                           f_ap_port->value().toInt());
}
