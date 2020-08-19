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
#include <App/App.h>

ProtocolMission::ProtocolMission(ProtocolVehicle *vehicle)
    : ProtocolBase(vehicle, "mission")
    , vehicle(vehicle)
{
    setTitle(tr("Mission"));
    setDescr(tr("Vehicle mission"));
    setIcon("map");
    setEnabled(false);

    connect(vehicle->nodes, &ProtocolNodes::nodeNotify, this, &ProtocolMission::nodeNotify);

    bindProperty(vehicle->nodes, "active");
}

void ProtocolMission::nodeNotify(ProtocolNode *protocol)
{
    connect(protocol,
            &ProtocolNode::filesAvailable,
            this,
            &ProtocolMission::filesChanged,
            Qt::UniqueConnection);
}
void ProtocolMission::filesChanged()
{
    int cnt = 0;
    for (auto n : vehicle->nodes->nodes()) {
        ProtocolNodeFile *f = n->file(nfile);
        if (!f)
            continue;
        cnt++;
        connect(f,
                &ProtocolNodeFile::downloaded,
                this,
                &ProtocolMission::fileDownloaded,
                Qt::UniqueConnection);
        connect(f,
                &ProtocolNodeFile::uploaded,
                this,
                &ProtocolMission::fileUploaded,
                Qt::UniqueConnection);
    }
    if (cnt > 0) {
        setEnabled(true);
        setValue(QString::number(cnt));
        if (!_enb) {
            _enb = true;
            emit available();
        }
    } else {
        setEnabled(false);
        setValue(QVariant());
    }
}

void ProtocolMission::upload(Mission d)
{
    QByteArray data = pack(d);

    for (auto n : vehicle->nodes->nodes()) {
        ProtocolNodeFile *f = n->file(nfile);
        if (!f)
            continue;
        qDebug() << "uploading mission to " << n->title();
        f->upload(data);
    }
}
void ProtocolMission::fileUploaded()
{
    qDebug() << "uploaded";
    emit uploaded();
}

void ProtocolMission::download()
{
    for (auto n : vehicle->nodes->nodes()) {
        ProtocolNodeFile *f = n->file(nfile);
        if (!f)
            continue;
        qDebug() << "downloading mission from " << n->title();
        f->download();
        break;
    }
}
void ProtocolMission::fileDownloaded(const xbus::node::file::info_s &info, const QByteArray data)
{
    Q_UNUSED(info)

    qDebug() << "downloaded";
    Mission d{};
    if (unpack(data, d)) {
        emit downloaded(d);
        return;
    }
    qWarning() << "error in mission";
}

QByteArray ProtocolMission::pack(const Mission &d)
{
    QByteArray data(65535, '\0');
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamWriter stream(pdata, data.size());

    xbus::mission::file_hdr_s fhdr{};
    fhdr.write(&stream); // will update later

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

    if (stream.pos() <= xbus::mission::Header::psize()) {
        qDebug() << "Upload empty mission";
    }

    xbus::mission::Header hdr;
    hdr.type = xbus::mission::Header::mi_stop;
    hdr.option = 0;
    hdr.write(&stream);
    data.resize(stream.pos());

    //update fhdr
    fhdr.size = stream.pos() - fhdr.psize();
    fhdr.hash = apx::crc32(stream.buffer() + fhdr.psize(), fhdr.size);

    strncpy(fhdr.title, d.title.toLocal8Bit(), sizeof(fhdr.title));
    fhdr.wp = d.waypoints.size();
    fhdr.rw = d.runways.size();
    fhdr.tw = d.taxiways.size();
    fhdr.pi = d.pois.size();

    stream.reset();
    fhdr.write(&stream);

    return data;
}

bool ProtocolMission::unpack(const QByteArray &data, Mission &d)
{
    if (data.size() < xbus::mission::file_hdr_s::psize())
        return false;

    //unpack mission
    uint16_t psize = static_cast<uint16_t>(data.size());
    const uint8_t *pdata = reinterpret_cast<const uint8_t *>(data.data());
    XbusStreamReader stream(pdata, psize);

    xbus::mission::file_hdr_s fhdr{};
    fhdr.read(&stream); // will update later
    d.title = QString(fhdr.title);

    qDebug() << d.title << data.size() << "bytes";

    if (fhdr.size != stream.available()) {
        qWarning() << "size" << fhdr.size << stream.available();
        return false;
    }

    int ecnt = 0, wpcnt = 0, rwcnt = 0;
    int lastWp = -1;

    while (stream.available() > 0) {
        ecnt++;
        xbus::mission::Header hdr;
        hdr.read(&stream);

        switch (hdr.type) {
        case xbus::mission::Header::mi_stop:
            stream.reset(psize); //finish
            ecnt--;
            continue;
        case xbus::mission::Header::mi_wp: {
            if (stream.available() < xbus::mission::Waypoint::psize())
                break;
            xbus::mission::Waypoint e;
            e.read(&stream);
            wpcnt++;
            Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            m.details["altitude"] = e.alt;
            m.details["type"] = waypointTypeToString(hdr.option);
            lastWp = d.waypoints.size();
            d.waypoints.append(m);
            continue;
        }
        case xbus::mission::Header::mi_action: { //wp actions
            QVariantMap a;
            switch (hdr.option) {
            default:
                break;
            case xbus::mission::Action::mo_speed: {
                xbus::mission::ActionSpeed e;
                e.read(&stream);
                a["speed"] = e.speed;
                break;
            }
            case xbus::mission::Action::mo_poi: {
                xbus::mission::ActionPoi e;
                e.read(&stream);
                a["poi"] = e.poi + 1;
                break;
            }
            case xbus::mission::Action::mo_scr: {
                xbus::mission::ActionScr e;
                e.read(&stream);
                a["script"] = QString(QByteArray(e.scr, sizeof(e.scr)));
                break;
            }
            case xbus::mission::Action::mo_loiter: {
                xbus::mission::ActionLoiter e;
                e.read(&stream);
                a["loiter"] = 1;
                a["radius"] = e.turnR;
                a["loops"] = e.loops;
                a["time"] = e.timeS;
                break;
            }
            case xbus::mission::Action::mo_shot: {
                xbus::mission::ActionShot e;
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
        case xbus::mission::Header::mi_rw: {
            if (stream.available() < xbus::mission::Runway::psize())
                break;
            xbus::mission::Runway e;
            e.read(&stream);
            rwcnt++;
            Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            m.details["hmsl"] = e.hmsl;
            m.details["dN"] = e.dN;
            m.details["dE"] = e.dE;
            m.details["approach"] = e.approach;
            m.details["type"] = runwayTypeToString(hdr.option);
            d.runways.append(m);
            continue;
        }
        case xbus::mission::Header::mi_tw: {
            if (stream.available() < xbus::mission::Taxiway::psize())
                break;
            xbus::mission::Taxiway e;
            e.read(&stream);
            Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            d.taxiways.append(m);
            continue;
        }
        case xbus::mission::Header::mi_pi: {
            if (stream.available() < xbus::mission::Poi::psize())
                break;
            xbus::mission::Poi e;
            e.read(&stream);
            Item m;
            m.lat = static_cast<qreal>(e.lat);
            m.lon = static_cast<qreal>(e.lon);
            m.details["hmsl"] = e.hmsl;
            m.details["radius"] = e.turnR;
            m.details["loops"] = e.loops;
            m.details["timeout"] = e.timeS;
            d.pois.append(m);
            continue;
        }
        case xbus::mission::Header::mi_emergency:
        case xbus::mission::Header::mi_restricted: {
            uint16_t sz = stream.available();
            if (sz < xbus::mission::Area::psize(0))
                break;
            xbus::mission::Area e;
            e.read(&stream);
            if (sz < xbus::mission::Area::psize(e.pointsCnt))
                break;
            //TODO - implement areas in GCS
            for (int i = 0; i < e.pointsCnt; ++i) {
                xbus::mission::Area::Point p;
                p.read(&stream);
            }
            continue;
        }
        }
        //error in mission
        return false;
    }
    return true;
}
