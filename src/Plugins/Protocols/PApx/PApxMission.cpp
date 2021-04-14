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
#include "PApxMission.h"

#include "PApxNode.h"
#include "PApxNodeFile.h"
#include "PApxNodes.h"
#include "PApxVehicle.h"

#include <App/App.h>

#include <xbus/XbusMission.h>

PApxMission::PApxMission(PApxVehicle *parent)
    : PMission(parent)
    , _vehicle(parent)
{
    // notify mission available on ident update if files found
    connect(parent->nodes(), &PNodes::node_available, this, [this](PNode *node) {
        connect(node, &PNode::identReceived, this, [this, node]() {
            if (static_cast<PApxNode *>(node)->file(file_name))
                emit missionAvailable();
        });
    });
}

void PApxMission::requestMission()
{
    auto f = _file();
    if (!f)
        return;
    connect(f, &PApxNodeFile::downloaded, this, &PApxMission::parseMissionData, Qt::UniqueConnection);
    new PApxNodeRequestFileRead(f->node(), file_name);
}

void PApxMission::updateMission(QVariant var)
{
    auto f = _file();
    if (!f)
        return;
    auto req = new PApxNodeRequestFileWrite(f->node(), file_name, _pack(var.value<QVariantMap>()));
    connect(req, &PApxNodeRequestFile::uploaded, this, &PMission::missionUpdated);
}

void PApxMission::parseMissionData(PApxNode *_node,
                                   const xbus::node::file::info_s &info,
                                   const QByteArray data)
{
    //qDebug() << "mission data" << info.size << data.size();
    PStreamReader stream(data);
    auto m = _unpack(stream);
    if (!m.isEmpty()) {
        m.insert("node_uid", _node->uid());
    }
    emit missionReceived(m);
}

PApxNodeFile *PApxMission::_file() const
{
    for (auto node : static_cast<PApxNodes *>(_vehicle->nodes())->nodes()) {
        auto f = node->file(file_name);
        if (f)
            return f;
    }
    apxMsgW() << tr("Mission source unavailable");
    return nullptr;
}
QVariantMap PApxMission::_unpack(PStreamReader &stream)
{
    //unpack mission
    if (stream.available() < xbus::mission::file_hdr_s::psize())
        return {};

    xbus::mission::file_hdr_s fhdr{};
    fhdr.read(&stream);
    QString title(QByteArray(fhdr.title, sizeof(fhdr.title)));

    qDebug() << title << stream.size() << "bytes";

    if (fhdr.size != stream.available()) {
        qWarning() << "data size" << fhdr.size << stream.available();
        return {};
    }

    QVariantList wp, rw, tw, pi;

    int ecnt = 0, wpcnt = 0, rwcnt = 0;

    while (stream.available() > 0) {
        ecnt++;
        xbus::mission::hdr_s hdr;
        hdr.read(&stream);

        switch (hdr.type) {
        case xbus::mission::STOP:
            stream.reset(stream.size()); //finish
            ecnt--;
            continue;
        case xbus::mission::WP: {
            if (stream.available() < xbus::mission::wp_s::psize())
                break;
            xbus::mission::wp_s e;
            e.read(&stream);
            wpcnt++;

            QVariantMap item;
            item.insert("lat", static_cast<qreal>(e.lat));
            item.insert("lon", static_cast<qreal>(e.lon));
            item.insert("altitude", e.alt);
            QString type = "direct";
            if (hdr.option == 1)
                type = "track";
            item.insert("type", type);

            wp.append(item);
            continue;
        }
        case xbus::mission::ACT: { //wp actions
            QVariantMap a;
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
                qWarning() << "Orphan actions in mission";
                continue;
            }
            // append actions to the last waypoint
            auto wpt = wp.last().value<QVariantMap>();
            auto actions = wpt.value("actions").value<QVariantMap>();
            for (auto i : a.keys())
                actions.insert(i, a[i]);
            wpt.insert("actions", actions);
            wp[wp.size() - 1] = wpt;
            continue;
        }
        case xbus::mission::RW: {
            if (stream.available() < xbus::mission::rw_s::psize())
                break;
            xbus::mission::rw_s e;
            e.read(&stream);
            rwcnt++;

            QVariantMap item;
            item.insert("lat", static_cast<qreal>(e.lat));
            item.insert("lon", static_cast<qreal>(e.lon));
            item.insert("hmsl", e.hmsl);
            item.insert("dN", e.dN);
            item.insert("dE", e.dE);
            item.insert("approach", e.approach);
            QString type = "left";
            if (hdr.option == 1)
                type = "right";
            item.insert("type", type);

            rw.append(item);
            continue;
        }
        case xbus::mission::TW: {
            if (stream.available() < xbus::mission::tw_s::psize())
                break;
            xbus::mission::tw_s e;
            e.read(&stream);

            QVariantMap item;
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
            QVariantMap item;
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
        return {};
    }

    if (ecnt <= 0)
        return {};

    QVariantMap m;

    if (!wp.isEmpty())
        m.insert("wp", wp);
    if (!rw.isEmpty())
        m.insert("rw", rw);
    if (!tw.isEmpty())
        m.insert("tw", tw);
    if (!pi.isEmpty())
        m.insert("pi", pi);

    if (m.isEmpty())
        return {};

    m.insert("title", title);
    return m;
}

QByteArray PApxMission::_pack(const QVariantMap &m)
{
    QByteArray data(65535, '\0');
    uint8_t *pdata = reinterpret_cast<uint8_t *>(data.data());
    XbusStreamWriter stream(pdata, data.size());

    xbus::mission::file_hdr_s fhdr{};
    fhdr.write(&stream); // will update later
    size_t pos_s = stream.pos();

    fhdr.off.wp = stream.pos() - pos_s;
    if (m.value("wp").canConvert<QVariantList>())
        for (auto const &i : m.value("wp").value<QSequentialIterable>()) {
            auto wp = i.value<QVariantMap>();
            xbus::mission::hdr_s hdr;
            hdr.type = xbus::mission::WP;
            hdr.option = 0;
            if (wp.value("type").toString().toLower() == "track")
                hdr.option = 1;
            hdr.write(&stream);
            xbus::mission::wp_s e;
            e.lat = wp.value("lat").toFloat();
            e.lon = wp.value("lon").toFloat();
            e.alt = wp.value("altitude").toUInt();
            e.write(&stream);
            fhdr.cnt.wp++;

            auto actions = wp.value("actions").value<QVariantMap>();
            if (actions.isEmpty())
                continue;

            if (actions.contains("speed")) {
                xbus::mission::hdr_s ahdr;
                ahdr.type = xbus::mission::ACT;
                ahdr.option = xbus::mission::ACT_SPEED;
                ahdr.write(&stream);
                xbus::mission::act_speed_s a;
                a.speed = actions.value("speed").toUInt();
                a.write(&stream);
            }
            if (actions.contains("poi")) {
                xbus::mission::hdr_s ahdr;
                ahdr.type = xbus::mission::ACT;
                ahdr.option = xbus::mission::ACT_PI;
                ahdr.write(&stream);
                xbus::mission::act_pi_s a;
                a.index = actions.value("poi").toUInt() - 1;
                a.write(&stream);
            }
            if (actions.contains("script")) {
                xbus::mission::hdr_s ahdr;
                ahdr.type = xbus::mission::ACT;
                ahdr.option = xbus::mission::ACT_SCR;
                ahdr.write(&stream);
                xbus::mission::act_scr_s a;
                QByteArray src(actions.value("script").toString().toUtf8());
                memset(a.scr, 0, sizeof(a.scr));
                memcpy(a.scr, src.data(), static_cast<size_t>(src.size()));
                a.scr[sizeof(a.scr) - 1] = 0;
                a.write(&stream);
            }
            if (actions.contains("shot")) {
                xbus::mission::hdr_s ahdr;
                ahdr.type = xbus::mission::ACT;
                ahdr.option = xbus::mission::ACT_SHOT;
                ahdr.write(&stream);
                xbus::mission::act_shot_s a;
                uint dshot = actions.value("dshot").toUInt();
                const QString shot = actions.value("shot").toString();
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
    if (m.value("rw").canConvert<QVariantList>())
        for (auto const &i : m.value("rw").value<QSequentialIterable>()) {
            auto rw = i.value<QVariantMap>();
            xbus::mission::hdr_s hdr;
            hdr.type = xbus::mission::RW;
            if (rw["type"].toString().toLower() == "right")
                hdr.option = 1;
            hdr.write(&stream);
            xbus::mission::rw_s e;
            e.lat = rw.value("lat").toFloat();
            e.lon = rw.value("lon").toFloat();
            e.hmsl = rw.value("hmsl").toInt();
            e.dN = rw.value("dN").toInt();
            e.dE = rw.value("dE").toInt();
            e.approach = rw.value("approach").toUInt();
            e.write(&stream);
            fhdr.cnt.rw++;
            //qDebug() << m.details;
        }

    fhdr.off.tw = stream.pos() - pos_s;
    if (m.value("tw").canConvert<QVariantList>())
        for (auto const &i : m.value("tw").value<QSequentialIterable>()) {
            auto tw = i.value<QVariantMap>();
            xbus::mission::hdr_s hdr;
            hdr.type = xbus::mission::TW;
            hdr.option = 0;
            hdr.write(&stream);
            xbus::mission::tw_s e;
            e.lat = tw.value("lat").toFloat();
            e.lon = tw.value("lon").toFloat();
            e.write(&stream);
            fhdr.cnt.tw++;
        }

    fhdr.off.pi = stream.pos() - pos_s;
    if (m.value("pi").canConvert<QVariantList>())
        for (auto const &i : m.value("pi").value<QSequentialIterable>()) {
            auto pi = i.value<QVariantMap>();
            xbus::mission::hdr_s hdr;
            hdr.type = xbus::mission::PI;
            hdr.option = 0;
            hdr.write(&stream);
            xbus::mission::pi_s e;
            e.lat = pi.value("lat").toFloat();
            e.lon = pi.value("lon").toFloat();
            e.hmsl = pi.value("hmsl").toInt();
            e.radius = pi.value("radius").toInt();
            e.loops = pi.value("loops").toUInt();
            e.timeout = AppRoot::timeFromString(pi.value("timeout").toString());
            e.write(&stream);
            fhdr.cnt.pi++;
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

    strncpy(fhdr.title, m.value("title").toString().toLocal8Bit(), sizeof(fhdr.title));

    stream.reset();
    fhdr.write(&stream);

    return data;
}
