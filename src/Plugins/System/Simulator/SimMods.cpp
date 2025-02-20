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
    , _pid_sim_sns(xbus::cmd::sim::sns)
{
    QString sect = tr("Jamming");

    f_enb = new Fact(this, "enb", tr("Enable Jamming"), tr("Intercept SIM sensors"), Bool);
    f_enb->setSection(sect);

    f_port_ap = new Fact(this,
                         "ap_port",
                         tr("Autopilot port"),
                         tr("UDP port to send modified sensors to"),
                         Int);
    f_port_ap->setSection(sect);
    f_port_ap->setMin(0);
    f_port_ap->setMax(65535);
    f_port_ap->setValue(UDP_PORT_AP_SNS + 2);

    f_mod = new SimMod(this);
    f_mod->setSection(sect);

    sect = tr("External JSON mods");
    f_ext_enb = new Fact(this,
                         "ext_enb",
                         tr("Enable External JSON mods"),
                         tr("Modify JSON over UDP"),
                         Bool);
    f_ext_enb->setSection(sect);

    f_port_jtx = new Fact(this,
                          "jtx_port",
                          tr("JSON TX port"),
                          tr("UDP port to send JSON data"),
                          Int);
    f_port_jtx->setSection(sect);

    f_port_jtx->setMin(0);
    f_port_jtx->setMax(65535);
    f_port_jtx->setValue(UDP_PORT_AP_SNS + 4);

    f_port_jrx = new Fact(this,
                          "jrx_port",
                          tr("JSON RX port"),
                          tr("UDP port to receive JSON data"),
                          Int);
    f_port_jrx->setSection(sect);

    f_port_jrx->setMin(0);
    f_port_jrx->setMax(65535);
    f_port_jrx->setValue(UDP_PORT_AP_SNS + 6);

    f_timeout_jrx = new Fact(this,
                             "jrx_timeout",
                             tr("JSON RX timeout"),
                             tr("Timeout to receive JSON data"),
                             Int);
    f_timeout_jrx->setSection(sect);
    f_timeout_jrx->setUnits("s");
    f_timeout_jrx->setMin(0);
    f_timeout_jrx->setMax(60);
    f_timeout_jrx->setValue(2);

    connect(f_enb, &Fact::valueChanged, this, [this]() {
        if (f_enb->value().toBool()) {
            _udp_sim.bind(UDP_PORT_AP_SNS);
        } else {
            _udp_sim.close();
        }
    });
    connect(&_udp_sim, &QUdpSocket::readyRead, this, &SimMods::simRead);

    connect(f_ext_enb, &Fact::valueChanged, this, [this]() {
        if (f_ext_enb->value().toBool()) {
            _udp_json.bind(f_port_jrx->value().toInt());
            _json_rx_time.start();
        } else {
            _udp_json.close();
        }
    });
    connect(&_udp_json, &QUdpSocket::readyRead, this, &SimMods::jsonRead);
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
                parse_sim_rx(_dec.data(), _dec.size());
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

void SimMods::parse_sim_rx(const void *data, size_t size)
{
    XbusStreamReader stream(data, size);
    if (stream.available() < xbus::pid_s::psize())
        return;
    xbus::pid_s pid;
    pid.read(&stream);

    mandala::uid_t uid = pid.uid;
    if (!xbus::cmd::sim::match(uid))
        return;

    // qDebug() << "RX" << size << uid;

    switch (uid) {
    default:
        qDebug() << "RX" << size << uid;
        break;
    case xbus::cmd::sim::sns:
        if (stream.available() < sizeof(mandala::bundle::sim_s)) {
            qWarning() << "RX" << size << uid << "too small";
            return;
        }

        // read simulated sensors data structure
        _pid_sim_sns = pid; // save for tx
        auto d = (mandala::bundle::sim_s *) stream.ptr();

        // apply modifications
        f_mod->modify(d);

        // check if JSON mods enabled
        if (!f_ext_enb->value().toBool())
            break;

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

        // qDebug() << json;

        // forward JSON to UDP_PORT_MOD_SNS
        _udp_json.writeDatagram(json, QHostAddress::LocalHost, f_port_jtx->value().toInt());

        // check for JSON RX timeout
        auto to = f_timeout_jrx->value().toInt();
        if (to > 0 && _json_rx_time.elapsed() > to * 1000) {
            // no JSON data received for a while
            // loopback to APX autopilot app as-is
            break;
        }

        return;
    }

    // forward to APX autopilot app as-is
    forwardToAP(data, size);
}

void SimMods::jsonRead()
{
    while (_udp_json.hasPendingDatagrams()) {
        QNetworkDatagram datagram = _udp_json.receiveDatagram();
        if (!datagram.isValid())
            continue;

        _json_rx_time.start();

        // qDebug() << datagram.data();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(datagram.data(), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << err.errorString();
            continue;
        }

        QVariantMap map = doc.toVariant().toMap();

        // qDebug() << map;

        // de-serialize bundle from variant map
        mandala::bundle::sim_s d;

        auto v = map["att_deg"].toList();
        d.att_deg[0] = v[0].toDouble();
        d.att_deg[1] = v[1].toDouble();
        d.att_deg[2] = v[2].toDouble();

        v = map["gyro_deg"].toList();
        d.gyro_deg[0] = v[0].toDouble();
        d.gyro_deg[1] = v[1].toDouble();
        d.gyro_deg[2] = v[2].toDouble();

        v = map["acc_ned"].toList();
        d.acc_ned[0] = v[0].toDouble();
        d.acc_ned[1] = v[1].toDouble();
        d.acc_ned[2] = v[2].toDouble();

        d.lat_deg = map["lat_deg"].toDouble();
        d.lon_deg = map["lon_deg"].toDouble();
        d.hmsl = map["hmsl"].toDouble();

        v = map["vel_ned"].toList();
        d.vel_ned[0] = v[0].toDouble();
        d.vel_ned[1] = v[1].toDouble();
        d.vel_ned[2] = v[2].toDouble();

        d.agl = map["agl"].toDouble();
        d.rpm = map["rpm"].toDouble();
        d.airspeed = map["airspeed"].toDouble();
        d.baro_mbar = map["baro_mbar"].toDouble();
        d.room_temp = map["room_temp"].toDouble();
        d.air_temp = map["air_temp"].toDouble();
        d.slip = map["slip"].toDouble();

        // qDebug() << d;

        // send modified structure to the autopilot
        XbusStreamWriter stream(_buf_sim_tx, sizeof(_buf_sim_tx));
        _pid_sim_sns.write(&stream); // saved from rx
        stream.write(&d, sizeof(d));

        forwardToAP(stream.buffer(), stream.pos());
    }
}

void SimMods::forwardToAP(const void *data, size_t size)
{
    auto cnt = _enc.encode(data, size);
    if (cnt <= 0)
        return;
    _udp_sim.writeDatagram(QByteArray((const char *) _enc.data(), cnt),
                           QHostAddress::LocalHost,
                           f_port_ap->value().toInt());
}
