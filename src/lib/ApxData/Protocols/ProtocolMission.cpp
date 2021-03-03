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
    QJsonValue json = unpack(data);
    if (json.isNull()) {
        qWarning() << "error in mission";
        return;
    }
    emit downloaded(json);
}

QByteArray ProtocolMission::pack(const Mission &d)
{
    QByteArray data(65535, '\0');
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamWriter stream(pdata, data.size());

    xbus::mission::file_hdr_s fhdr{};
    fhdr.write(&stream); // will update later
    size_t pos_s = stream.pos();

    fhdr.off.wp = stream.pos() - pos_s;
    for (int i = 0; i < d.waypoints.size(); ++i) {
        const ProtocolMission::Item &m = d.waypoints.at(i);
        xbus::mission::hdr_s hdr;
        hdr.type = xbus::mission::WP;
        hdr.option = ProtocolMission::waypointTypeFromString(m.details.value("type").toString());
        hdr.write(&stream);
        xbus::mission::wp_s e;
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
            xbus::mission::hdr_s ahdr;
            ahdr.type = xbus::mission::ACT;
            ahdr.option = xbus::mission::ACT_SPEED;
            ahdr.write(&stream);
            xbus::mission::act_speed_s a;
            a.speed = amap.value("speed").toUInt();
            a.write(&stream);
        }
        if (!amap.value("poi").isEmpty()) {
            xbus::mission::hdr_s ahdr;
            ahdr.type = xbus::mission::ACT;
            ahdr.option = xbus::mission::ACT_PI;
            ahdr.write(&stream);
            xbus::mission::act_pi_s a;
            a.index = amap.value("poi").toUInt() - 1;
            a.write(&stream);
        }
        if (!amap.value("script").isEmpty()) {
            xbus::mission::hdr_s ahdr;
            ahdr.type = xbus::mission::ACT;
            ahdr.option = xbus::mission::ACT_SCR;
            ahdr.write(&stream);
            xbus::mission::act_scr_s a;
            QByteArray src(amap.value("script").toUtf8());
            memset(a.scr, 0, sizeof(a.scr));
            memcpy(a.scr, src.data(), static_cast<size_t>(src.size()));
            a.scr[sizeof(a.scr) - 1] = 0;
            a.write(&stream);
        }
        if (!amap.value("loiter").isEmpty()) {
            xbus::mission::hdr_s ahdr;
            ahdr.type = xbus::mission::ACT;
            ahdr.option = xbus::mission::ACT_LOITER;
            ahdr.write(&stream);
            xbus::mission::act_loiter_s a;
            a.radius = amap.value("radius").toInt();
            a.loops = amap.value("loops").toUInt();
            a.timeout = AppRoot::timeFromString(amap.value("time"));
            a.write(&stream);
        }
        if (!amap.value("shot").isEmpty()) {
            xbus::mission::hdr_s ahdr;
            ahdr.type = xbus::mission::ACT;
            ahdr.option = xbus::mission::ACT_SHOT;
            ahdr.write(&stream);
            xbus::mission::act_shot_s a;
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

    fhdr.off.rw = stream.pos() - pos_s;
    for (int i = 0; i < d.runways.size(); ++i) {
        const Item &m = d.runways.at(i);
        xbus::mission::hdr_s hdr;
        hdr.type = xbus::mission::RW;
        hdr.option = runwayTypeFromString(m.details.value("type").toString());
        hdr.write(&stream);
        xbus::mission::rw_s e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.hmsl = m.details.value("hmsl").toInt();
        e.dN = m.details.value("dN").toInt();
        e.dE = m.details.value("dE").toInt();
        e.approach = m.details.value("approach").toUInt();
        e.write(&stream);
        //qDebug() << m.details;
    }

    fhdr.off.tw = stream.pos() - pos_s;
    for (int i = 0; i < d.taxiways.size(); ++i) {
        const ProtocolMission::Item &m = d.taxiways.at(i);
        xbus::mission::hdr_s hdr;
        hdr.type = xbus::mission::TW;
        hdr.option = 0;
        hdr.write(&stream);
        xbus::mission::tw_s e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.write(&stream);
    }

    fhdr.off.pi = stream.pos() - pos_s;
    for (int i = 0; i < d.pois.size(); ++i) {
        const ProtocolMission::Item &m = d.pois.at(i);
        xbus::mission::hdr_s hdr;
        hdr.type = xbus::mission::PI;
        hdr.option = 0;
        hdr.write(&stream);
        xbus::mission::pi_s e;
        e.lat = static_cast<float>(m.lat);
        e.lon = static_cast<float>(m.lon);
        e.hmsl = m.details.value("hmsl").toInt();
        e.radius = m.details.value("radius").toInt();
        e.loops = m.details.value("loops").toUInt();
        e.timeout = AppRoot::timeFromString(m.details.value("timeout").toString());
        e.write(&stream);
    }

    if (stream.pos() <= xbus::mission::hdr_s::psize()) {
        qDebug() << "Upload empty mission";
    }

    xbus::mission::hdr_s hdr;
    hdr.type = xbus::mission::STOP;
    hdr.option = 0;
    hdr.write(&stream);
    data.resize(stream.pos());

    //update fhdr
    fhdr.size = stream.pos() - pos_s;
    //fhdr.hash = apx::crc32(stream.buffer() + pos_s, fhdr.size);

    strncpy(fhdr.title, d.title.toLocal8Bit(), sizeof(fhdr.title));
    fhdr.cnt.wp = d.waypoints.size();
    fhdr.cnt.rw = d.runways.size();
    fhdr.cnt.tw = d.taxiways.size();
    fhdr.cnt.pi = d.pois.size();

    stream.reset();
    fhdr.write(&stream);

    return data;
}

QJsonValue ProtocolMission::unpack(const QByteArray &data)
{
    if (data.size() < xbus::mission::file_hdr_s::psize())
        return false;

    //unpack mission
    uint16_t psize = static_cast<uint16_t>(data.size());
    const uint8_t *pdata = reinterpret_cast<const uint8_t *>(data.data());
    XbusStreamReader stream(pdata, psize);

    xbus::mission::file_hdr_s fhdr{};
    fhdr.read(&stream);
    QString title(QByteArray(fhdr.title, sizeof(fhdr.title)));

    qDebug() << title << data.size() << "bytes";

    if (fhdr.size != stream.available()) {
        qWarning() << "size" << fhdr.size << stream.available();
        return QJsonValue();
    }

    QJsonArray wp, rw, tw, pi;

    int ecnt = 0, wpcnt = 0, rwcnt = 0;

    while (stream.available() > 0) {
        ecnt++;
        xbus::mission::hdr_s hdr;
        hdr.read(&stream);

        switch (hdr.type) {
        case xbus::mission::STOP:
            stream.reset(psize); //finish
            ecnt--;
            continue;
        case xbus::mission::WP: {
            if (stream.available() < xbus::mission::wp_s::psize())
                break;
            xbus::mission::wp_s e;
            e.read(&stream);
            wpcnt++;

            QJsonObject item;
            item.insert("lat", static_cast<qreal>(e.lat));
            item.insert("lon", static_cast<qreal>(e.lon));
            item.insert("altitude", e.alt);
            item.insert("type", waypointTypeToString(hdr.option));

            wp.append(item);
            continue;
        }
        case xbus::mission::ACT: { //wp actions
            QJsonObject a;
            switch (hdr.option) {
            default:
                break;
            case xbus::mission::ACT_SPEED: {
                xbus::mission::act_speed_s e;
                e.read(&stream);
                a.insert("speed", e.speed);
                break;
            }
            case xbus::mission::ACT_PI: {
                xbus::mission::act_pi_s e;
                e.read(&stream);
                a.insert("poi", e.index + 1);
                break;
            }
            case xbus::mission::ACT_SCR: {
                xbus::mission::act_scr_s e;
                e.read(&stream);
                a.insert("script", QString(QByteArray(e.scr, sizeof(e.scr))));
                break;
            }
            case xbus::mission::ACT_SHOT: {
                xbus::mission::act_shot_s e;
                e.read(&stream);
                switch (e.opt) {
                case 0: //single
                    a.insert("shot", "single");
                    a.insert("dshot", 0);
                    break;
                case 1: //start
                    a.insert("shot", "start");
                    a.insert("dshot", e.dist);
                    break;
                case 2: //stop
                    a.insert("shot", "stop");
                    a.insert("dshot", 0);
                    break;
                }
                break;
            }
            }
            if (wp.isEmpty()) {
                apxMsgW() << tr("Orphan actions in mission");
                continue;
            }
            QJsonObject wpt = wp.last().toObject();
            if (!wpt.contains("actions"))
                wpt.insert("actions", QJsonObject());
            QJsonObject actions = wpt["actions"].toObject();
            for (auto i : a.keys())
                actions.insert(i, a[i]);
            wpt.insert("actions", actions);
            QJsonValueRef ref = wp[wp.size() - 1];
            ref = wpt;
            continue;
        }
        case xbus::mission::RW: {
            if (stream.available() < xbus::mission::rw_s::psize())
                break;
            xbus::mission::rw_s e;
            e.read(&stream);
            rwcnt++;

            QJsonObject item;
            item.insert("lat", static_cast<qreal>(e.lat));
            item.insert("lon", static_cast<qreal>(e.lon));
            item.insert("hmsl", e.hmsl);
            item.insert("dN", e.dN);
            item.insert("dE", e.dE);
            item.insert("approach", e.approach);
            item.insert("type", runwayTypeToString(hdr.option));

            rw.append(item);
            continue;
        }
        case xbus::mission::TW: {
            if (stream.available() < xbus::mission::tw_s::psize())
                break;
            xbus::mission::tw_s e;
            e.read(&stream);

            QJsonObject item;
            item.insert("lat", static_cast<qreal>(e.lat));
            item.insert("lon", static_cast<qreal>(e.lon));

            tw.append(item);
            continue;
        }
        case xbus::mission::PI: {
            if (stream.available() < xbus::mission::pi_s::psize())
                break;
            xbus::mission::pi_s e;
            e.read(&stream);
            QJsonObject item;
            item.insert("lat", static_cast<qreal>(e.lat));
            item.insert("lon", static_cast<qreal>(e.lon));
            item.insert("hmsl", e.hmsl);
            item.insert("radius", e.radius);
            item.insert("loops", e.loops);
            item.insert("timeout", e.timeout);

            pi.append(item);
            continue;
        }
        case xbus::mission::EMG:
        case xbus::mission::DIS: {
            uint16_t sz = stream.available();
            size_t pointsCnt = hdr.option;
            if (sz < xbus::mission::area_s::psize(pointsCnt))
                break;
            //TODO - implement areas in GCS
            for (size_t i = 0; i < pointsCnt; ++i) {
                xbus::mission::area_s p;
                p.read(&stream);
            }
            continue;
        }
        }
        //error in mission
        return false;
    }

    QJsonObject json;
    json.insert("title", title);

    QJsonObject items;
    if (!wp.isEmpty())
        items.insert("wp", wp);
    if (!rw.isEmpty())
        items.insert("rw", rw);
    if (!tw.isEmpty())
        items.insert("tw", tw);
    if (!pi.isEmpty())
        items.insert("pi", pi);

    json.insert("items", items);

    return json;
}
