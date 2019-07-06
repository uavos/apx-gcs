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

#include <node.h>
#include <Mandala.h>
#include <Mission.h>
#include <ApxLog.h>
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
    uint data_cnt = static_cast<uint>(data.size());
    int ecnt = 0, wpcnt = 0, rwcnt = 0;
    const quint8 *ptr = reinterpret_cast<const quint8 *>(data.data());
    for (uint cnt = 0; data_cnt >= sizeof(Mission::_item_hdr); data_cnt -= cnt) {
        ptr += cnt;
        ecnt++;
        const Mission::_item_hdr *hdr = reinterpret_cast<const Mission::_item_hdr *>(ptr);
        switch (hdr->type) {
        case Mission::mi_stop:
            data_cnt -= sizeof(Mission::_item_hdr);
            ecnt--;
            break;
        case Mission::mi_wp: {
            cnt = sizeof(Mission::_item_wp);
            if (data_cnt < cnt)
                break;
            wpcnt++;
            const Mission::_item_wp *e = reinterpret_cast<const Mission::_item_wp *>(ptr);
            DictMission::Item m;
            m.lat = static_cast<qreal>(e->lat);
            m.lon = static_cast<qreal>(e->lon);
            m.details["altitude"] = e->alt;
            m.details["type"] = DictMission::waypointTypeToString(e->hdr.option);
            //wp actions
            QVariantMap a;
            bool err = false;
            while ((data_cnt - cnt) >= sizeof(Mission::_item_hdr)
                   && (reinterpret_cast<const Mission::_item_hdr *>(ptr + cnt))->type
                          == Mission::mi_action) {
                const Mission::_item_action *v = reinterpret_cast<const Mission::_item_action *>(
                    ptr + cnt);
                uint sz = static_cast<uint>(Mission::action_size(v->hdr.option));
                if ((data_cnt - cnt) < sz) {
                    err = true;
                    break; //error
                }
                cnt += sz;
                switch (v->hdr.option) {
                case Mission::mo_speed:
                    a["speed"] = v->speed;
                    break;
                case Mission::mo_poi:
                    a["poi"] = v->poi + 1;
                    break;
                case Mission::mo_scr:
                    a["script"] = QString(v->scr);
                    break;
                case Mission::mo_loiter:
                    a["loiter"] = 1;
                    a["turnR"] = v->loiter.turnR;
                    a["loops"] = v->loiter.loops;
                    a["time"] = v->loiter.timeS;
                    break;
                case Mission::mo_shot: {
                    switch (v->shot.opt) {
                    case 0: //single
                        a["shot"] = "single";
                        a["dshot"] = 0;
                        break;
                    case 1: //start
                        a["shot"] = "start";
                        a["dshot"] = v->shot.dist;
                        break;
                    case 2: //stop
                        a["shot"] = "stop";
                        a["dshot"] = 0;
                        break;
                    }
                } break;
                default:
                    err = true;
                    break;
                }
            }
            if (err)
                break;
            QStringList st;
            foreach (const QString &k, a.keys()) {
                st.append(QString("%1=%2").arg(k).arg(a.value(k).toString()));
            }
            m.details["actions"] = st.join(',');
            d.waypoints.append(m);
        }
            continue;
        case Mission::mi_rw: {
            const Mission::_item_rw *e = reinterpret_cast<const Mission::_item_rw *>(ptr);
            cnt = sizeof(Mission::_item_rw);
            if (data_cnt < cnt)
                break;
            rwcnt++;
            DictMission::Item m;
            m.lat = static_cast<qreal>(e->lat);
            m.lon = static_cast<qreal>(e->lon);
            m.details["hmsl"] = e->hmsl;
            m.details["dN"] = e->dN;
            m.details["dE"] = e->dE;
            m.details["approach"] = e->approach;
            m.details["type"] = DictMission::runwayTypeToString(e->hdr.option);
            d.runways.append(m);
        }
            continue;
        case Mission::mi_tw: {
            const Mission::_item_tw *e = reinterpret_cast<const Mission::_item_tw *>(ptr);
            cnt = sizeof(Mission::_item_tw);
            if (data_cnt < cnt)
                break;
            DictMission::Item m;
            m.lat = static_cast<qreal>(e->lat);
            m.lon = static_cast<qreal>(e->lon);
            d.taxiways.append(m);
        }
            continue;
        case Mission::mi_pi: {
            const Mission::_item_pi *e = reinterpret_cast<const Mission::_item_pi *>(ptr);
            cnt = sizeof(Mission::_item_pi);
            if (data_cnt < cnt)
                break;
            DictMission::Item m;
            m.lat = static_cast<qreal>(e->lat);
            m.lon = static_cast<qreal>(e->lon);
            m.details["hmsl"] = e->hmsl;
            m.details["radius"] = e->turnR;
            m.details["loops"] = e->loops;
            m.details["timeout"] = e->timeS;
            d.pois.append(m);
        }
            continue;
        case Mission::mi_action: { //ignore, loaded in wp
            const Mission::_item_action *e = reinterpret_cast<const Mission::_item_action *>(ptr);
            cnt = Mission::action_size(e->hdr.option);
            if (data_cnt < cnt)
                break;
        }
            continue;
        case Mission::mi_restricted: {
            const Mission::_item_area *e = reinterpret_cast<const Mission::_item_area *>(ptr);
            cnt = Mission::area_size(e->pointsCnt);
            if (data_cnt < cnt)
                break;
        }
            continue;
        case Mission::mi_emergency: {
            const Mission::_item_area *e = reinterpret_cast<const Mission::_item_area *>(ptr);
            cnt = Mission::area_size(e->pointsCnt);
            if (data_cnt < cnt)
                break;
        }
            continue;
        }
        break;
    }
    if (data_cnt)
        emit missionDataError();
    else
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
    QByteArray ba;
    for (int i = 0; i < d.runways.size(); ++i) {
        const DictMission::Item &m = d.runways.at(i);
        Mission::_item_rw v;
        v.hdr.type = Mission::mi_rw;
        v.hdr.option = DictMission::runwayTypeFromString(m.details.value("type").toString());
        v.lat = static_cast<float>(m.lat);
        v.lon = static_cast<float>(m.lon);
        v.hmsl = m.details.value("hmsl").toInt();
        v.dN = m.details.value("dN").toInt();
        v.dE = m.details.value("dE").toInt();
        v.approach = m.details.value("approach").toUInt();
        ba.append(reinterpret_cast<const char *>(&v), sizeof(v));
    }
    for (int i = 0; i < d.waypoints.size(); ++i) {
        const DictMission::Item &m = d.waypoints.at(i);
        Mission::_item_wp v;
        v.hdr.type = Mission::mi_wp;
        v.hdr.option = DictMission::waypointTypeFromString(m.details.value("type").toString());
        v.lat = static_cast<float>(m.lat);
        v.lon = static_cast<float>(m.lon);
        v.alt = m.details.value("altitude").toInt();
        ba.append(reinterpret_cast<const char *>(&v), sizeof(v));
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
            Mission::_item_action a;
            a.hdr.type = Mission::mi_action;
            a.hdr.option = Mission::mo_speed;
            a.speed = amap.value("speed").toUInt();
            ba.append(reinterpret_cast<const char *>(&a),
                      static_cast<int>(Mission::action_size(a.hdr.option)));
        }
        if (!amap.value("poi").isEmpty()) {
            Mission::_item_action a;
            a.hdr.type = Mission::mi_action;
            a.hdr.option = Mission::mo_poi;
            a.poi = amap.value("poi").toUInt() - 1;
            ba.append(reinterpret_cast<const char *>(&a),
                      static_cast<int>(Mission::action_size(a.hdr.option)));
        }
        if (!amap.value("script").isEmpty()) {
            Mission::_item_action a;
            a.hdr.type = Mission::mi_action;
            a.hdr.option = Mission::mo_scr;
            strncpy(a.scr, amap.value("script").toUtf8().data(), sizeof(Mission::_item_action::scr));
            ba.append(reinterpret_cast<const char *>(&a),
                      static_cast<int>(Mission::action_size(a.hdr.option)));
        }
        if (!amap.value("loiter").isEmpty()) {
            Mission::_item_action a;
            a.hdr.type = Mission::mi_action;
            a.hdr.option = Mission::mo_loiter;
            a.loiter.turnR = amap.value("turnR").toInt();
            a.loiter.loops = amap.value("loops").toUInt();
            a.loiter.timeS = amap.value("time").toUInt();
            ba.append(reinterpret_cast<const char *>(&a),
                      static_cast<int>(Mission::action_size(a.hdr.option)));
        }
        if (!amap.value("shot").isEmpty()) {
            Mission::_item_action a;
            a.hdr.type = Mission::mi_action;
            a.hdr.option = Mission::mo_shot;
            uint dshot = amap.value("dshot").toUInt();
            const QString &shot = amap.value("shot");
            a.shot.opt = 0;
            a.shot.dist = 0;
            if (shot == "start") {
                if (dshot == 0) {
                    apxMsgW() << tr("Auto shot distance is zero");
                    break;
                }
                a.shot.opt = 1;
                if (dshot > ((1 << 12) - 1))
                    dshot = ((1 << 12) - 1);
                a.shot.dist = static_cast<int16_t>(dshot);
            } else if (shot == "stop") {
                a.shot.opt = 2;
                a.shot.dist = 0;
            } else if (shot != "single") {
                apxMsgW() << tr("Unknown shot mode") << shot;
            }
            ba.append(reinterpret_cast<const char *>(&a),
                      static_cast<int>(Mission::action_size(a.hdr.option)));
        }
    }
    for (int i = 0; i < d.taxiways.size(); ++i) {
        const DictMission::Item &m = d.taxiways.at(i);
        Mission::_item_tw v;
        v.hdr.type = Mission::mi_tw;
        v.lat = static_cast<float>(m.lat);
        v.lon = static_cast<float>(m.lon);
        ba.append(reinterpret_cast<const char *>(&v), sizeof(v));
    }
    for (int i = 0; i < d.pois.size(); ++i) {
        const DictMission::Item &m = d.pois.at(i);
        Mission::_item_pi v;
        v.hdr.type = Mission::mi_pi;
        v.lat = static_cast<float>(m.lat);
        v.lon = static_cast<float>(m.lon);
        v.hmsl = m.details.value("hmsl").toInt();
        v.turnR = m.details.value("radius").toInt();
        v.loops = m.details.value("loops").toUInt();
        v.timeS = m.details.value("timeout").toUInt();
        ba.append(reinterpret_cast<const char *>(&v), sizeof(v));
    }

    if (!ba.size()) {
        qDebug() << "Can't upload empty mission";
    }

    Mission::_item_hdr v;
    memset(&v, 0, sizeof(v));
    v.type = Mission::mi_stop;
    //if(seqList.size()>1 && currentSeq>0) v.option=1;
    //else v.option=0;
    ba.append(reinterpret_cast<const char *>(&v), sizeof(v));
    emit sendUplink(ba);
}
//=============================================================================
//=============================================================================
