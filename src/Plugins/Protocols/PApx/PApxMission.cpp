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
#include "PApxUnit.h"

#include <App/App.h>

#include <XbusMission.h>

#include <crc.h>

PApxMission::PApxMission(PApxUnit *parent)
    : PMission(parent)
    , _unit(parent)
{
    // notify mission available on ident update if files found
    connect(parent->nodes(), &PNodes::node_available, this, [this](PNode *node) {
        connect(node, &PNode::identReceived, this, &PApxMission::updateFiles);
    });
}

void PApxMission::updateFiles()
{
    auto f = _file();
    if (!f)
        return;
    connect(f, &PApxNodeFile::downloaded, this, &PApxMission::parseMissionData, Qt::UniqueConnection);
    connect(f, &PApxNodeFile::uploaded, this, &PApxMission::parseMissionData, Qt::UniqueConnection);
    emit missionAvailable();
}

void PApxMission::requestMission()
{
    auto f = _file();
    if (!f) {
        apxMsgW() << tr("Mission source unavailable");
        return;
    }
    new PApxNodeRequestFileRead(f->node(), file_name);
}

void PApxMission::updateMission(QVariant var)
{
    auto f = _file();
    if (!f) {
        apxMsgW() << tr("Mission storage unavailable");
        return;
    }
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
    for (auto node : static_cast<PApxNodes *>(_unit->nodes())->nodes()) {
        auto f = node->file(file_name);
        if (f)
            return f;
    }
    return nullptr;
}

static void _unpack_act(QVariantMap *actions, const QVariantList &act, int index)
{
    auto a = act.value(index - 1).toMap();
    if (a.isEmpty())
        return;

    auto name = a.keys().first();
    auto value = a.value(name);
    if (name == "next") {
        for (const auto i : value.toList()) {
            _unpack_act(actions, act, i.toInt());
        }
    } else {
        actions->insert(name, value);
    }
}

QVariantMap PApxMission::_unpack(PStreamReader &stream)
{
    //unpack mission
    if (stream.available() < sizeof(xbus::mission::file_hdr_s))
        return {};

    const auto fsize = stream.available();
    xbus::mission::file_hdr_s fhdr{};
    stream.read(&fhdr, sizeof(fhdr));

    //check file format
    if (fhdr.format != xbus::mission::file_hdr_s::FORMAT) {
        qWarning() << "file format" << fhdr.format;
        return {};
    }

    // check size
    if (fhdr.size != fsize) {
        qWarning() << "data size" << fhdr.size << fsize;
        return {};
    }
    if (fhdr.pld_offset < sizeof(fhdr) || fhdr.pld_offset >= fsize) {
        qWarning() << "payload offset" << fhdr.pld_offset << fsize;
        return {};
    }
    stream.reset(fhdr.pld_offset);

    // check payload crc32
    const auto pld_crc32 = apx::crc32(stream.ptr(), fsize - fhdr.pld_offset);
    if (fhdr.pld_crc32 != pld_crc32) {
        qWarning() << "payload crc32 mismatch" << pld_crc32 << fhdr.pld_crc32;
        return {};
    }

    // get title
    const auto title = QString::fromUtf8(
        QByteArray(fhdr.title, strnlen(fhdr.title, sizeof(fhdr.title))));

    // qDebug() << title << stream.size() << "bytes";

    // read items
    QVariantList rw, pi, wp, tw, act;

    // Runways
    stream.reset(fhdr.pld_offset + fhdr.items.rw.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < fhdr.items.rw.cnt; ++i) {
        xbus::mission::rw_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading rw" << i << fhdr.items.rw.cnt;
            return {};
        }
        QVariantMap m;
        m.insert("lat", mandala::a32_to_deg(e.lat));
        m.insert("lon", mandala::a32_to_deg(e.lon));
        m.insert("hmsl", e.hmsl);
        m.insert("dN", e.dN);
        m.insert("dE", e.dE);
        m.insert("approach", e.approach);
        switch (e.type) {
        case xbus::mission::rw_s::CLEFT:
            m.insert("type", "left");
            break;
        case xbus::mission::rw_s::CRIGHT:
            m.insert("type", "right");
            break;
        }
        rw.append(m);
    }

    // Points of Interest
    stream.reset(fhdr.pld_offset + fhdr.items.pi.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < fhdr.items.pi.cnt; ++i) {
        xbus::mission::pi_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading pi" << i << fhdr.items.pi.cnt;
            return {};
        }
        QVariantMap m;
        m.insert("lat", mandala::a32_to_deg(e.lat));
        m.insert("lon", mandala::a32_to_deg(e.lon));
        m.insert("hmsl", e.hmsl);
        m.insert("radius", e.radius);
        m.insert("loops", e.loops);
        m.insert("timeout", e.timeout);
        pi.append(m);
    }

    // Actions
    stream.reset(fhdr.pld_offset + fhdr.items.act.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < fhdr.items.act.cnt; ++i) {
        xbus::mission::act_s e;
        auto pos_s = stream.pos();
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading act" << i << fhdr.items.act.cnt;
            return {};
        }
        stream.reset(pos_s);
        QVariantMap m;
        switch (e.type) {
        case xbus::mission::act_s::ACT_SEQ: {
            xbus::mission::act_seq_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_seq" << i << fhdr.items.act.cnt;
                return {};
            }
            QVariantList next;
            for (size_t j = 0; j < sizeof(e.next) / sizeof(e.next[0]); ++j) {
                if (e.next[j] == 0)
                    break;
                next.append(QVariant::fromValue((uint) e.next[j]));
            }
            m.insert("next", next);
            break;
        }
        case xbus::mission::act_s::ACT_SPEED: {
            xbus::mission::act_speed_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_speed" << i << fhdr.items.act.cnt;
                return {};
            }
            m.insert("speed", QVariant::fromValue((uint) e.speed));
            break;
        }
        case xbus::mission::act_s::ACT_POI: {
            xbus::mission::act_poi_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_poi" << i << fhdr.items.act.cnt;
                return {};
            }
            m.insert("poi", QVariant::fromValue((uint) e.index));
            break;
        }
        case xbus::mission::act_s::ACT_SCR: {
            xbus::mission::act_scr_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_scr" << i << fhdr.items.act.cnt;
                return {};
            }
            m.insert("script", QString::fromUtf8(QByteArray(e.scr, strnlen(e.scr, sizeof(e.scr)))));
            break;
        }
        }
        act.append(m);
    }

    // Waypoints
    stream.reset(fhdr.pld_offset + fhdr.items.wp.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < fhdr.items.wp.cnt; ++i) {
        xbus::mission::wp_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading wp" << i << fhdr.items.wp.cnt;
            return {};
        }
        QVariantMap m;
        m.insert("lat", mandala::a32_to_deg(e.lat));
        m.insert("lon", mandala::a32_to_deg(e.lon));
        m.insert("alt", e.alt);
        m.insert("amsl", e.amsl);
        m.insert("xtrk", e.xtrk);
        m.insert("vtrk", e.vtrk);
        if (e.act) {
            QVariantMap actions;
            _unpack_act(&actions, act, e.act);
            if (!actions.isEmpty())
                m.insert("actions", actions);
        }
        wp.append(m);
    }

    // Taxiways
    stream.reset(fhdr.pld_offset + fhdr.items.tw.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < fhdr.items.tw.cnt; ++i) {
        xbus::mission::tw_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading tw" << i << fhdr.items.tw.cnt;
            return {};
        }
        QVariantMap m;
        m.insert("lat", mandala::a32_to_deg(e.lat));
        m.insert("lon", mandala::a32_to_deg(e.lon));
        tw.append(m);
    }

    QVariantMap m;

    if (!rw.isEmpty())
        m.insert("rw", rw);
    if (!pi.isEmpty())
        m.insert("pi", pi);
    if (!wp.isEmpty())
        m.insert("wp", wp);
    if (!tw.isEmpty())
        m.insert("tw", tw);

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

    // skip header for now
    xbus::mission::file_hdr_s fhdr{};
    stream.reset(fhdr.pld_offset);

    // write items

    // Runways
    fhdr.items.rw.off = stream.pos() - fhdr.pld_offset;
    fhdr.items.rw.cnt = 0;
    for (auto i : m.value("rw").toList()) {
        const auto im = i.toMap();
        xbus::mission::rw_s e{};
        e.lat = mandala::deg_to_a32(im.value("lat").toDouble());
        e.lon = mandala::deg_to_a32(im.value("lon").toDouble());
        e.hmsl = im.value("hmsl").toInt();
        e.dN = im.value("dN").toInt();
        e.dE = im.value("dE").toInt();
        const auto approach = im.value("approach").toUInt();
        e.approach = approach > xbus::mission::rw_s::APPROACH_MAX
                         ? xbus::mission::rw_s::APPROACH_MAX
                         : approach;
        const auto type = im.value("type").toString().toLower();
        if (type == "left")
            e.type = xbus::mission::rw_s::CLEFT;
        else if (type == "right")
            e.type = xbus::mission::rw_s::CRIGHT;
        stream.write(&e, sizeof(e));
        fhdr.items.rw.cnt++;
    }

    // Points of Interest
    fhdr.items.pi.off = stream.pos() - fhdr.pld_offset;
    fhdr.items.pi.cnt = 0;
    for (auto i : m.value("pi").toList()) {
        const auto im = i.toMap();
        xbus::mission::pi_s e{};
        e.lat = mandala::deg_to_a32(im.value("lat").toDouble());
        e.lon = mandala::deg_to_a32(im.value("lon").toDouble());
        e.hmsl = im.value("hmsl").toInt();
        e.radius = im.value("radius").toInt();
        e.loops = im.value("loops").toUInt();
        e.timeout = AppRoot::timeFromString(im.value("timeout").toString(), false);
        stream.write(&e, sizeof(e));
        fhdr.items.pi.cnt++;
    }

    // Waypoints
    QVariantList act; // wil collect actions here
    fhdr.items.wp.off = stream.pos() - fhdr.pld_offset;
    fhdr.items.wp.cnt = 0;
    for (auto i : m.value("wp").toList()) {
        const auto im = i.toMap();
        xbus::mission::wp_s e{};
        e.lat = mandala::deg_to_a32(im.value("lat").toDouble());
        e.lon = mandala::deg_to_a32(im.value("lon").toDouble());
        e.alt = im.value("alt").toInt();
        e.amsl = im.value("amsl").toBool();
        e.xtrk = im.value("xtrk").toBool();
        e.vtrk = im.value("vtrk").toBool();
        const auto actions = im.value("actions").toMap();
        if (!actions.isEmpty()) {
            // fill act list with missing items
            QVariantList seq;
            for (auto key : actions.keys()) {
                uint seq_index = 0;
                const auto value = actions.value(key);
                for (int act_index = 0; act_index < act.size(); act_index++) {
                    if (act.at(act_index).toMap().value(key).toString() != value.toString())
                        continue;
                    // found in act array
                    seq_index = act_index + 1;
                    break;
                }
                if (seq_index == 0) {
                    // not found in act array
                    // add entry to act array
                    act.append(QVariantMap({{key, value}}));
                    seq_index = act.size();
                }
                seq.append(seq_index);
            }
            const auto seq_max = xbus::mission::act_seq_s::MAX;
            uint seq_index = 0;
            if (seq.size() == 1) {
                seq_index = seq.at(0).toUInt();
            } else if (seq.size() > xbus::mission::act_seq_s::MAX) {
                qWarning() << "too many actions in sequence";
            } else {
                // write sequence of actions
                // find existing sequence in act list
                uint seq_index = 0;
                QString key = "seq";
                for (int act_index = 0; act_index < act.size(); act_index++) {
                    if (act.at(act_index).toMap().value(key) != act)
                        continue;
                    // found in act array
                    seq_index = act_index + 1;
                    break;
                }
                if (seq_index == 0) {
                    // not found in act array
                    // add entry to act array
                    act.append(QVariantMap({{key, seq}}));
                    seq_index = act.size();
                }
            }
            e.act = seq_index;
        }
        stream.write(&e, sizeof(e));
        fhdr.items.wp.cnt++;
    }

    // Taxiways
    fhdr.items.tw.off = stream.pos() - fhdr.pld_offset;
    fhdr.items.tw.cnt = 0;
    for (auto i : m.value("tw").toList()) {
        const auto im = i.toMap();
        xbus::mission::tw_s e{};
        e.lat = mandala::deg_to_a32(im.value("lat").toDouble());
        e.lon = mandala::deg_to_a32(im.value("lon").toDouble());
        stream.write(&e, sizeof(e));
        fhdr.items.tw.cnt++;
    }

    // Actions
    fhdr.items.act.off = stream.pos() - fhdr.pld_offset;
    fhdr.items.act.cnt = 0;
    for (const auto &i : act) {
        const auto im = i.toMap();
        const auto key = im.keys().first();
        const auto value = im.value(key);
        if (key == "seq") {
            xbus::mission::act_seq_s e{};
            e.type = xbus::mission::act_s::ACT_SEQ;
            const auto next = value.toList();
            for (int j = 0; j < next.size() && j < xbus::mission::act_seq_s::MAX; ++j) {
                e.next[j] = next.at(j).toUInt();
            }
            stream.write(&e, sizeof(e));
        } else if (key == "speed") {
            xbus::mission::act_speed_s e{};
            e.type = xbus::mission::act_s::ACT_SPEED;
            e.speed = value.toUInt();
            stream.write(&e, sizeof(e));
        } else if (key == "poi") {
            xbus::mission::act_poi_s e{};
            e.type = xbus::mission::act_s::ACT_POI;
            e.index = value.toUInt();
            stream.write(&e, sizeof(e));
        } else if (key == "script") {
            xbus::mission::act_scr_s e{};
            e.type = xbus::mission::act_s::ACT_SCR;
            auto scr = value.toString();
            if (scr.size() > sizeof(e.scr) - 1)
                scr.resize(sizeof(e.scr) - 1);
            memcpy(e.scr, scr.toUtf8().data(), scr.size());
            e.scr[sizeof(e.scr) - 1] = 0; // ensure null-terminated
            stream.write(&e, sizeof(e));
        } else {
            qWarning() << "Unknown action" << key;
            continue;
        }
        fhdr.items.act.cnt++;
    }

    if (stream.pos() <= fhdr.pld_offset) {
        qDebug() << "Upload empty mission";
    }

    //update fhdr
    fhdr.size = stream.pos();
    fhdr.pld_crc32 = apx::crc32(pdata + fhdr.pld_offset, fhdr.size - fhdr.pld_offset, 0xFFFFFFFF);

    strncpy(fhdr.title, m.value("title").toString().toUtf8(), sizeof(fhdr.title) - 1);

    stream.reset();
    stream.write(&fhdr, sizeof(fhdr));

    return data.left(fhdr.size);
}
