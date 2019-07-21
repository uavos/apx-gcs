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

#include <Xbus/XbusMissionPayload.h>

#include <ApxLog.h>
#include <App/AppRoot.h>
//=============================================================================
ProtocolMission::ProtocolMission(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle)
{
    connect(this, &ProtocolMission::sendUplink, vehicle, &ProtocolVehicle::sendMissionRequest);
    connect(vehicle, &ProtocolVehicle::missionData, this, &ProtocolMission::missionData);
}
//=============================================================================
//=============================================================================
//=============================================================================
void ProtocolMission::missionData(QByteArray data)
{
    if (data.size() < 4)
        return;
    qDebug() << data.size() << "bytes";
    //unpack mission
    DictMission::Mission d;
    int ecnt = 0, wpcnt = 0, rwcnt = 0;

    uint16_t psize = static_cast<uint16_t>(data.size());
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamReader stream(pdata, psize);

    int lastWp = -1;

    while (stream.position() < psize) {
        ecnt++;
        XbusMissionPayload::Header hdr;
        hdr.read(&stream);

        switch (hdr.type) {
        case XbusMissionPayload::Header::mi_stop:
            stream.reset(psize); //finish
            ecnt--;
            continue;
        case XbusMissionPayload::Header::mi_wp: {
            if (stream.tailSize() < XbusMissionPayload::Waypoint::psize())
                break;
            XbusMissionPayload::Waypoint e;
            e.read(&stream);
            wpcnt++;
            DictMission::Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            m.details["altitude"] = e.alt;
            m.details["type"] = DictMission::waypointTypeToString(hdr.option);
            lastWp = d.waypoints.size();
            d.waypoints.append(m);
            continue;
        }
        case XbusMissionPayload::Header::mi_action: { //wp actions
            QVariantMap a;
            switch (hdr.option) {
            default:
                break;
            case XbusMissionPayload::Action::mo_speed: {
                XbusMissionPayload::ActionSpeed e;
                e.read(&stream);
                a["speed"] = e.speed;
                break;
            }
            case XbusMissionPayload::Action::mo_poi: {
                XbusMissionPayload::ActionPoi e;
                e.read(&stream);
                a["poi"] = e.poi + 1;
                break;
            }
            case XbusMissionPayload::Action::mo_scr: {
                XbusMissionPayload::ActionScr e;
                e.read(&stream);
                a["script"] = QString(QByteArray(e.scr.data(), e.scr.size()));
                break;
            }
            case XbusMissionPayload::Action::mo_loiter: {
                XbusMissionPayload::ActionLoiter e;
                e.read(&stream);
                a["loiter"] = 1;
                a["radius"] = e.turnR;
                a["loops"] = e.loops;
                a["time"] = e.timeS;
                break;
            }
            case XbusMissionPayload::Action::mo_shot: {
                XbusMissionPayload::ActionShot e;
                e.read(&stream);
                switch (e.opt) {
                case 0: //single
                    a["shot"] = "single";
                    a["dshot"] = 0;
                    break;
                case 1: //start
                    a["shot"] = "start";
                    a["dshot"] = e.dist;
                    break;
                case 2: //stop
                    a["shot"] = "stop";
                    a["dshot"] = 0;
                    break;
                }
                break;
            }
            }
            if (lastWp < 0) {
                apxMsgW() << tr("Orphan actions in mission");
                continue;
            }
            QStringList st;
            foreach (const QString &k, a.keys()) {
                st.append(QString("%1=%2").arg(k).arg(a.value(k).toString()));
            }
            d.waypoints[lastWp].details["actions"] = st.join(',');
            continue;
        }
        case XbusMissionPayload::Header::mi_rw: {
            if (stream.tailSize() < XbusMissionPayload::Runway::psize())
                break;
            XbusMissionPayload::Runway e;
            e.read(&stream);
            rwcnt++;
            DictMission::Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            m.details["hmsl"] = e.hmsl;
            m.details["dN"] = e.dN;
            m.details["dE"] = e.dE;
            m.details["approach"] = e.approach;
            m.details["type"] = DictMission::runwayTypeToString(hdr.option);
            d.runways.append(m);
            continue;
        }
        case XbusMissionPayload::Header::mi_tw: {
            if (stream.tailSize() < XbusMissionPayload::Taxiway::psize())
                break;
            XbusMissionPayload::Taxiway e;
            e.read(&stream);
            DictMission::Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            d.taxiways.append(m);
            continue;
        }
        case XbusMissionPayload::Header::mi_pi: {
            if (stream.tailSize() < XbusMissionPayload::Poi::psize())
                break;
            XbusMissionPayload::Poi e;
            e.read(&stream);
            DictMission::Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            m.details["hmsl"] = e.hmsl;
            m.details["radius"] = e.turnR;
            m.details["loops"] = e.loops;
            m.details["timeout"] = e.timeS;
            d.pois.append(m);
            continue;
        }
        case XbusMissionPayload::Header::mi_emergency:
        case XbusMissionPayload::Header::mi_restricted: {
            uint16_t sz = stream.tailSize();
            if (sz < XbusMissionPayload::Area::psize(0))
                break;
            XbusMissionPayload::Area e;
            e.read(&stream);
            if (sz < XbusMissionPayload::Area::psize(e.pointsCnt))
                break;
            //TODO - implement areas in GCS
            for (int i = 0; i < e.pointsCnt; ++i) {
                XbusMissionPayload::Area::Point p;
                p.read(&stream);
            }
            continue;
        }
        }
        //error in mission
        emit missionDataError();
        return;
    }
    emit missionDataReceived(d);
}
//=============================================================================
void ProtocolMission::downloadMission()
{
    emit sendUplink(QByteArray());
}
//=============================================================================
//=============================================================================
void ProtocolMission::missionDataUpload(DictMission::Mission d)
{
    QByteArray data(1024, '\0');
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamWriter stream(pdata);

    for (int i = 0; i < d.runways.size(); ++i) {
        const DictMission::Item &m = d.runways.at(i);
        XbusMissionPayload::Header hdr;
        hdr.type = XbusMissionPayload::Header::mi_rw;
        hdr.option = DictMission::runwayTypeFromString(m.details.value("type").toString());
        hdr.write(&stream);
        XbusMissionPayload::Runway e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.hmsl = m.details.value("hmsl").toInt();
        e.dN = m.details.value("dN").toInt();
        e.dE = m.details.value("dE").toInt();
        e.approach = m.details.value("approach").toUInt();
        e.write(&stream);
    }
    for (int i = 0; i < d.waypoints.size(); ++i) {
        const DictMission::Item &m = d.waypoints.at(i);
        XbusMissionPayload::Header hdr;
        hdr.type = XbusMissionPayload::Header::mi_wp;
        hdr.option = DictMission::waypointTypeFromString(m.details.value("type").toString());
        hdr.write(&stream);
        XbusMissionPayload::Waypoint e;
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
            XbusMissionPayload::Header ahdr;
            ahdr.type = XbusMissionPayload::Header::mi_action;
            ahdr.option = XbusMissionPayload::Action::mo_speed;
            ahdr.write(&stream);
            XbusMissionPayload::ActionSpeed a;
            a.speed = amap.value("speed").toUInt();
            a.write(&stream);
        }
        if (!amap.value("poi").isEmpty()) {
            XbusMissionPayload::Header ahdr;
            ahdr.type = XbusMissionPayload::Header::mi_action;
            ahdr.option = XbusMissionPayload::Action::mo_poi;
            ahdr.write(&stream);
            XbusMissionPayload::ActionPoi a;
            a.poi = amap.value("poi").toUInt() - 1;
            a.write(&stream);
        }
        if (!amap.value("script").isEmpty()) {
            XbusMissionPayload::Header ahdr;
            ahdr.type = XbusMissionPayload::Header::mi_action;
            ahdr.option = XbusMissionPayload::Action::mo_scr;
            ahdr.write(&stream);
            XbusMissionPayload::ActionScr a;
            QByteArray src(amap.value("script").toUtf8());
            std::copy(src.begin(), src.end(), a.scr.begin());
            a.scr[a.scr.size() - 1] = 0;
            a.write(&stream);
        }
        if (!amap.value("loiter").isEmpty()) {
            XbusMissionPayload::Header ahdr;
            ahdr.type = XbusMissionPayload::Header::mi_action;
            ahdr.option = XbusMissionPayload::Action::mo_loiter;
            ahdr.write(&stream);
            XbusMissionPayload::ActionLoiter a;
            a.turnR = amap.value("radius").toInt();
            a.loops = amap.value("loops").toUInt();
            a.timeS = AppRoot::timeFromString(amap.value("time"));
            a.write(&stream);
        }
        if (!amap.value("shot").isEmpty()) {
            XbusMissionPayload::Header ahdr;
            ahdr.type = XbusMissionPayload::Header::mi_action;
            ahdr.option = XbusMissionPayload::Action::mo_shot;
            ahdr.write(&stream);
            XbusMissionPayload::ActionShot a;
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
        const DictMission::Item &m = d.taxiways.at(i);
        XbusMissionPayload::Header hdr;
        hdr.type = XbusMissionPayload::Header::mi_tw;
        hdr.option = 0;
        hdr.write(&stream);
        XbusMissionPayload::Taxiway e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.write(&stream);
    }
    for (int i = 0; i < d.pois.size(); ++i) {
        const DictMission::Item &m = d.pois.at(i);
        XbusMissionPayload::Header hdr;
        hdr.type = XbusMissionPayload::Header::mi_pi;
        hdr.option = 0;
        hdr.write(&stream);
        XbusMissionPayload::Poi e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.hmsl = m.details.value("hmsl").toInt();
        e.turnR = m.details.value("radius").toInt();
        e.loops = m.details.value("loops").toUInt();
        e.timeS = AppRoot::timeFromString(m.details.value("timeout").toString());
        e.write(&stream);
    }

    if (stream.position() <= XbusMissionPayload::Header::psize()) {
        qDebug() << "Can't upload empty mission";
    }

    XbusMissionPayload::Header hdr;
    hdr.type = XbusMissionPayload::Header::mi_stop;
    hdr.option = 0;
    hdr.write(&stream);
    data.resize(stream.position());
    emit sendUplink(data);
}
//=============================================================================
//=============================================================================
