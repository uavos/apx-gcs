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
#include "ProtocolMission.h"
#include "ProtocolVehicle.h"

ProtocolMission::ProtocolMission(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle, "mission")
    , _vehicle(vehicle)
{
    setTitle(tr("Mission"));
    setDescr(tr("Vehicle mission"));
    setIcon("map");
    setDataType(Count);
}

void ProtocolMission::download() {}

void ProtocolMission::upload(Mission d)
{
    QByteArray data(1024, '\0');
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamWriter stream(pdata, data.size());
    /*
    for (int i = 0; i < d.runways.size(); ++i) {
        const Item &m = d.runways.at(i);
        xbus::mission::Header hdr;
        hdr.type = xbus::mission::Header::mi_rw;
        hdr.option = runwayTypeFromString(m.details.value("type").toString());
        hdr.write(&stream);
        xbus::mission::Runway e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.hmsl = m.details.value("hmsl").toInt();
        e.dN = m.details.value("dN").toInt();
        e.dE = m.details.value("dE").toInt();
        e.approach = m.details.value("approach").toUInt();
        e.write(&stream);
    }
    for (int i = 0; i < d.waypoints.size(); ++i) {
        const ProtocolMission::Item &m = d.waypoints.at(i);
        xbus::mission::Header hdr;
        hdr.type = xbus::mission::Header::mi_wp;
        hdr.option = ProtocolMission::waypointTypeFromString(m.details.value("type").toString());
        hdr.write(&stream);
        xbus::mission::Waypoint e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.alt = m.details.value("altitude").toInt();
        e.write(&stream);
        if (m.details.value("actions").toString().isEmpty())
            continue;
        QMap<QString, QString> amap;
        foreach (const QString &s, m.details.value("actions").toString().split(',')) {
            int i = s.indexOf('=');
            if (i <= 0)
                continue;
            amap[s.left(i)] = s.mid(i + 1);
        }

        if (!amap.value("speed").isEmpty()) {
            xbus::mission::Header ahdr;
            ahdr.type = xbus::mission::Header::mi_action;
            ahdr.option = xbus::mission::Action::mo_speed;
            ahdr.write(&stream);
            xbus::mission::ActionSpeed a;
            a.speed = amap.value("speed").toUInt();
            a.write(&stream);
        }
        if (!amap.value("poi").isEmpty()) {
            xbus::mission::Header ahdr;
            ahdr.type = xbus::mission::Header::mi_action;
            ahdr.option = xbus::mission::Action::mo_poi;
            ahdr.write(&stream);
            xbus::mission::ActionPoi a;
            a.poi = amap.value("poi").toUInt() - 1;
            a.write(&stream);
        }
        if (!amap.value("script").isEmpty()) {
            xbus::mission::Header ahdr;
            ahdr.type = xbus::mission::Header::mi_action;
            ahdr.option = xbus::mission::Action::mo_scr;
            ahdr.write(&stream);
            xbus::mission::ActionScr a;
            QByteArray src(amap.value("script").toUtf8());
            memset(a.scr, 0, sizeof(a.scr));
            memcpy(a.scr, src.data(), static_cast<size_t>(src.size()));
            a.scr[sizeof(a.scr) - 1] = 0;
            a.write(&stream);
        }
        if (!amap.value("loiter").isEmpty()) {
            xbus::mission::Header ahdr;
            ahdr.type = xbus::mission::Header::mi_action;
            ahdr.option = xbus::mission::Action::mo_loiter;
            ahdr.write(&stream);
            xbus::mission::ActionLoiter a;
            a.turnR = amap.value("radius").toInt();
            a.loops = amap.value("loops").toUInt();
            a.timeS = AppRoot::timeFromString(amap.value("time"));
            a.write(&stream);
        }
        if (!amap.value("shot").isEmpty()) {
            xbus::mission::Header ahdr;
            ahdr.type = xbus::mission::Header::mi_action;
            ahdr.option = xbus::mission::Action::mo_shot;
            ahdr.write(&stream);
            xbus::mission::ActionShot a;
            uint dshot = amap.value("dshot").toUInt();
            const QString shot = amap.value("shot");
            a.opt = 0;
            a.dist = 0;
            if (shot == "start") {
                if (dshot == 0) {
                    apxMsgW() << tr("Auto shot distance is zero");
                    break;
                }
                a.opt = 1;
                if (dshot > ((1 << 12) - 1))
                    dshot = ((1 << 12) - 1);
                a.dist = static_cast<int16_t>(dshot);
            } else if (shot == "stop") {
                a.opt = 2;
                a.dist = 0;
            } else if (shot != "single") {
                apxMsgW() << tr("Unknown shot mode") << shot;
            }
            a.write(&stream);
        }
    }
    for (int i = 0; i < d.taxiways.size(); ++i) {
        const ProtocolMission::Item &m = d.taxiways.at(i);
        xbus::mission::Header hdr;
        hdr.type = xbus::mission::Header::mi_tw;
        hdr.option = 0;
        hdr.write(&stream);
        xbus::mission::Taxiway e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.write(&stream);
    }
    for (int i = 0; i < d.pois.size(); ++i) {
        const ProtocolMission::Item &m = d.pois.at(i);
        xbus::mission::Header hdr;
        hdr.type = xbus::mission::Header::mi_pi;
        hdr.option = 0;
        hdr.write(&stream);
        xbus::mission::Poi e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.hmsl = m.details.value("hmsl").toInt();
        e.turnR = m.details.value("radius").toInt();
        e.loops = m.details.value("loops").toUInt();
        e.timeS = AppRoot::timeFromString(m.details.value("timeout").toString());
        e.write(&stream);
    }

    if (stream.position() <= xbus::mission::Header::psize()) {
        qDebug() << "Can't upload empty mission";
    }

    xbus::mission::Header hdr;
    hdr.type = xbus::mission::Header::mi_stop;
    hdr.option = 0;
    hdr.write(&stream);
    data.resize(stream.position());*/
}
