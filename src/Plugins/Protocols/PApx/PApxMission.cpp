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
#include <ApxMisc/JsonHelpers.h>

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

static void _unpack_act(QVariantMap *wp, const QVariantList &act, int index)
{
    auto a = act.value(index - 1).toMap();
    if (a.isEmpty())
        return;

    const auto k = a.keys().first();
    if (k == "seq") {
        for (const auto &i : a.value(k).toList())
            _unpack_act(wp, act, i.toInt());
        return;
    }
    // merge action to wp
    for (auto it = a.begin(); it != a.end(); ++it)
        wp->insert(it.key(), it.value());
}
static void _pack_act_wp(QVariantMap *actions,
                         const QVariantMap &wp,
                         const QString &name,
                         QStringList names = {})
{
    if (names.isEmpty())
        names = {name};
    QVariantMap m;
    for (const auto &k : names) {
        auto v = wp.value(k);
        if (v.isNull())
            continue;
        m.insert(k, v);
    }
    if (!m.isEmpty())
        actions->insert(name, m);
}
static uint _pack_act_add(QVariantList *actions_list, const QVariantMap &action)
{
    // action must be only one name-value pair
    if (action.size() != 1)
        return 0;
    // fill global actions list with missing items
    auto index = actions_list->indexOf(action);
    if (index >= 0)
        return index + 1;
    if (actions_list->size() >= 256) {
        qWarning() << "too many actions";
        return 0;
    }
    // add new action to global list
    actions_list->append(action);
    return actions_list->size();
}

static uint _pack_act(QVariantList *actions_list, const QVariantMap &wp)
{
    // group wp params to actions
    QVariantMap m;
    _pack_act_wp(&m, wp, "alt", {"altitude", "amsl", "atrack"});
    _pack_act_wp(&m, wp, "trk", {"xtrack"});
    _pack_act_wp(&m, wp, "speed");
    _pack_act_wp(&m, wp, "poi");
    _pack_act_wp(&m, wp, "script");

    if (m.isEmpty())
        return 0;

    if (m.size() == 1)
        return _pack_act_add(actions_list, m);

    // add a seq action
    QVariantList seq;
    for (auto it = m.begin(); it != m.end(); ++it) {
        QVariantMap a{{it.key(), it.value()}};
        auto index = _pack_act_add(actions_list, a);
        if (!index)
            return 0;
        seq.append(index);
    }
    return _pack_act_add(actions_list, {{"seq", seq}});
}

QVariantMap PApxMission::_unpack(PStreamReader &stream)
{
    //unpack mission
    if (stream.available() < sizeof(xbus::mission::file_hdr_s))
        return {};

    const auto file_size = stream.available();
    xbus::mission::file_hdr_s file_hdr{};
    stream.read(&file_hdr, sizeof(file_hdr));

    //check file format
    if (file_hdr.format != xbus::mission::file_hdr_s::FORMAT) {
        qWarning() << "file format" << file_hdr.format;
        return {};
    }

    // check size
    if (file_hdr.size != file_size) {
        qWarning() << "data size" << file_hdr.size << file_size;
        return {};
    }

    if (stream.available() <= sizeof(xbus::mission::pld_hdr_s)) {
        qWarning() << "empty file" << stream.available();
        return {};
    }

    // check payload crc32
    const auto pld_crc32 = apx::crc32(stream.ptr(),
                                      file_size - sizeof(xbus::mission::file_hdr_s),
                                      0xFFFFFFFF);
    if (file_hdr.pld_crc32 != pld_crc32) {
        qWarning() << "payload crc32 mismatch" << pld_crc32 << file_hdr.pld_crc32;
        return {};
    }

    // read payload header
    xbus::mission::pld_hdr_s hdr{};
    stream.read(&hdr, sizeof(hdr));
    const auto pld_offset = stream.pos();

    // get title
    const auto title = QString::fromUtf8(
        QByteArray(hdr.title, strnlen(hdr.title, sizeof(hdr.title))));

    // qDebug() << title << stream.size() << "bytes";

    // read items
    QVariantList rw, pi, wp, tw, act;

    // Runways
    stream.reset(pld_offset + hdr.items.rw.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < hdr.items.rw.cnt; ++i) {
        xbus::mission::rw_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading rw" << i << hdr.items.rw.cnt;
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
    stream.reset(pld_offset + hdr.items.pi.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < hdr.items.pi.cnt; ++i) {
        xbus::mission::pi_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading pi" << i << hdr.items.pi.cnt;
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
    stream.reset(pld_offset + hdr.items.act.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < hdr.items.act.cnt; ++i) {
        xbus::mission::act_s e;
        auto pos_s = stream.pos();
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading act" << i << hdr.items.act.cnt;
            return {};
        }
        stream.reset(pos_s);
        QVariantMap m;
        switch (e.type) {
        case xbus::mission::act_s::ACT_SEQ: {
            xbus::mission::act_seq_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_seq" << i << hdr.items.act.cnt;
                return {};
            }
            QVariantList seq;
            for (uint j = 0; j < e.cnt; ++j) {
                uint8_t index;
                if (stream.read(&index, 1) != 1) {
                    qWarning() << "error reading act_seq item" << i << hdr.items.act.cnt << j
                               << e.cnt;
                    return {};
                }
                seq.append((uint) index);
            }
            m.insert("seq", seq);
            break;
        }
        case xbus::mission::act_s::ATR_ALT: {
            xbus::mission::act_alt_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_alt" << i << hdr.items.act.cnt;
                return {};
            }
            m.insert("altitude", e.alt);
            m.insert("amsl", e.amsl);
            m.insert("atrack", e.atrk);
            break;
        }
        case xbus::mission::act_s::ATR_TRK: {
            xbus::mission::act_trk_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_trk" << i << hdr.items.act.cnt;
                return {};
            }
            m.insert("xtrack", e.xtrk);
            break;
        }
        case xbus::mission::act_s::TRG_SPEED: {
            xbus::mission::act_speed_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_speed" << i << hdr.items.act.cnt;
                return {};
            }
            m.insert("speed", QVariant::fromValue((uint) e.speed));
            break;
        }
        case xbus::mission::act_s::TRG_POI: {
            xbus::mission::act_poi_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_poi" << i << hdr.items.act.cnt;
                return {};
            }
            m.insert("poi", QVariant::fromValue((uint) e.index));
            break;
        }
        case xbus::mission::act_s::TRG_SCR: {
            xbus::mission::act_scr_s e;
            if (stream.read(&e, sizeof(e)) != sizeof(e)) {
                qWarning() << "error reading act_scr" << i << hdr.items.act.cnt;
                return {};
            }
            auto s = stream.read_string(xbus::mission::act_scr_s::MAX);
            m.insert("script", QString::fromUtf8(QByteArray(s, strlen(s))));
            break;
        }
        }
        act.append(m);
    }
    // json::save("act-unpack", QJsonDocument::fromVariant(act).array());

    // Waypoints
    stream.reset(pld_offset + hdr.items.wp.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < hdr.items.wp.cnt; ++i) {
        xbus::mission::wp_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading wp" << i << hdr.items.wp.cnt;
            return {};
        }
        QVariantMap m;
        m.insert("lat", mandala::a32_to_deg(e.lat));
        m.insert("lon", mandala::a32_to_deg(e.lon));
        if (e.act) {
            _unpack_act(&m, act, e.act);
            // qDebug() << "wp" << e.act << m;
        }
        wp.append(m);
    }

    // Taxiways
    stream.reset(pld_offset + hdr.items.tw.off);
    if (!stream.available())
        return {};
    for (size_t i = 0; i < hdr.items.tw.cnt; ++i) {
        xbus::mission::tw_s e;
        if (stream.read(&e, sizeof(e)) != sizeof(e)) {
            qWarning() << "error reading tw" << i << hdr.items.tw.cnt;
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
    xbus::mission::pld_hdr_s hdr{};
    const auto pld_offset = sizeof(xbus::mission::file_hdr_s) + sizeof(xbus::mission::pld_hdr_s);
    stream.reset(pld_offset);

    // write items

    // Runways
    hdr.items.rw.off = stream.pos() - pld_offset;
    hdr.items.rw.cnt = 0;
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
        hdr.items.rw.cnt++;
    }

    // Points of Interest
    hdr.items.pi.off = stream.pos() - pld_offset;
    hdr.items.pi.cnt = 0;
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
        hdr.items.pi.cnt++;
    }

    // Waypoints
    QVariantList actions_list; // wil collect actions here
    hdr.items.wp.off = stream.pos() - pld_offset;
    hdr.items.wp.cnt = 0;
    for (auto i : m.value("wp").toList()) {
        auto im = i.toMap();
        xbus::mission::wp_s e{};
        e.lat = mandala::deg_to_a32(im.take("lat").toDouble());
        e.lon = mandala::deg_to_a32(im.take("lon").toDouble());
        e.act = _pack_act(&actions_list, im);
        stream.write(&e, sizeof(e));
        hdr.items.wp.cnt++;
    }
    // json::save("act-pack", QJsonDocument::fromVariant(actions_list).array());

    // Taxiways
    hdr.items.tw.off = stream.pos() - pld_offset;
    hdr.items.tw.cnt = 0;
    for (auto i : m.value("tw").toList()) {
        const auto im = i.toMap();
        xbus::mission::tw_s e{};
        e.lat = mandala::deg_to_a32(im.value("lat").toDouble());
        e.lon = mandala::deg_to_a32(im.value("lon").toDouble());
        stream.write(&e, sizeof(e));
        hdr.items.tw.cnt++;
    }

    // Actions
    hdr.items.act.off = stream.pos() - pld_offset;
    hdr.items.act.cnt = 0;
    for (const auto &i : actions_list) {
        const auto im = i.toMap();
        const auto key = im.keys().first();
        const auto value = im.value(key);
        auto m = value.toMap();
        if (key == "seq") {
            xbus::mission::act_seq_s e{};
            e.type = xbus::mission::act_s::ACT_SEQ;
            auto list = value.toList();
            e.cnt = list.size();
            stream.write(&e, sizeof(e));
            for (int j = 0; j < list.size(); ++j) {
                uint8_t index = list.at(j).toUInt();
                stream.write(&index, 1);
            }
        } else if (key == "alt") {
            xbus::mission::act_alt_s e{};
            e.type = xbus::mission::act_s::ATR_ALT;
            e.alt = m.value("altitude").toUInt();
            e.amsl = m.value("amsl").toBool();
            e.atrk = m.value("atrack").toBool();
            stream.write(&e, sizeof(e));
        } else if (key == "trk") {
            xbus::mission::act_trk_s e{};
            e.type = xbus::mission::act_s::ATR_TRK;
            e.xtrk = m.value("xtrack").toBool();
            stream.write(&e, sizeof(e));
        } else if (key == "speed") {
            xbus::mission::act_speed_s e{};
            e.type = xbus::mission::act_s::TRG_SPEED;
            e.speed = m.value(key).toUInt();
            stream.write(&e, sizeof(e));
        } else if (key == "poi") {
            xbus::mission::act_poi_s e{};
            e.type = xbus::mission::act_s::TRG_POI;
            e.index = m.value(key).toUInt();
            stream.write(&e, sizeof(e));
        } else if (key == "script") {
            xbus::mission::act_scr_s e{};
            e.type = xbus::mission::act_s::TRG_SCR;
            stream.write(&e, sizeof(e));
            auto scr = m.value(key).toString();
            auto max = xbus::mission::act_scr_s::MAX;
            if (scr.size() > max - 1)
                scr.resize(max - 1);
            stream.write_string(scr.toUtf8().constData());
        } else {
            qWarning() << "Unknown action" << key;
            continue;
        }
        hdr.items.act.cnt++;
    }

    if (stream.pos() <= pld_offset) {
        qDebug() << "Upload empty mission";
    }

    //update hdr
    strncpy(hdr.title, m.value("title").toString().toUtf8(), sizeof(hdr.title) - 1);
    uint32_t pld_crc32 = apx::crc32(&hdr, sizeof(hdr), 0xFFFFFFFF);
    pld_crc32 = apx::crc32(pdata + pld_offset, stream.pos() - pld_offset, pld_crc32);

    xbus::mission::file_hdr_s file_hdr{};
    file_hdr.size = stream.pos();
    file_hdr.pld_crc32 = pld_crc32;

    stream.reset();
    stream.write(&file_hdr, sizeof(file_hdr));
    stream.write(&hdr, sizeof(hdr));

    return data.left(file_hdr.size);
}
