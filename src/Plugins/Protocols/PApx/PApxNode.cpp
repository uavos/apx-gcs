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
#include "PApxNode.h"
#include "PApxNodeFile.h"
#include "PApxNodes.h"

#include <Mandala/Mandala.h>

#include <xbus/XbusMission.h>
#include <xbus/XbusNode.h>
#include <xbus/XbusScript.h>

PApxNode::PApxNode(PApxNodes *parent, QString uid)
    : PNode(parent, uid)
    , _req(this)
{
    // store ident to parse dict
    connect(this, &PNode::identReceived, this, [this](QJsonValue json) {
        _dict_hash = json.toObject().value("hash").toInt();
    });
}

PApxNode::~PApxNode()
{
    clear_requests();
}

void PApxNode::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    mandala::uid_t uid = pid.uid;

    if (uid == mandala::cmd::env::nmt::search::uid) {
        requestIdent();
        return;
    }

    //filter zero sized responses if any
    if (stream.available() == 0)
        return;

    // file ops - make them download data from any source
    if (uid == mandala::cmd::env::nmt::file::uid) {
        // accept both pid requests and responses
        size_t spos = stream.pos();

        if (stream.available() <= sizeof(xbus::node::file::op_e))
            return;

        xbus::node::file::op_e op;
        stream >> op;

        if (op & xbus::node::file::reply_op_mask)
            trace()->block("re");
        trace()->block(QString::number(op & ~xbus::node::file::reply_op_mask));

        const char *s = stream.read_string(16);
        if (!s)
            return;

        trace()->block(QString(s));
        trace()->data(stream.payload());

        auto f = file(s);
        if (!f)
            return;

        f->process_downlink(op, stream);

        stream.reset(spos);
    }

    // node messages
    if (uid == mandala::cmd::env::nmt::msg::uid) {
        if (stream.available() < (sizeof(xbus::node::msg::type_e) + 1))
            return;

        xbus::node::msg::type_e t;
        stream >> t;
        trace()->block(QString::number(t));

        const char *s = stream.read_string(stream.available());
        QString msg(QString(s).trimmed());
        trace()->block(msg);

        if (msg.isEmpty())
            return;

        msg.replace(":", ": ");
        msg = msg.simplified();

        emit messageReceived((msg_type_e) t, msg);
        return;
    }

    // check for requests responses
    if (pid.pri != xbus::pri_response) {
        // a request from another GCS?
        return;
    }

    // check for pending request's response
    for (auto req : _requests) {
        if (req->uid() != uid)
            continue;
        // there's a request waiting for the packet's uid
        if (req->check_response(stream)) {
            delete_request(req);
        }
    }
}

void PApxNode::schedule_request(PApxNodeRequest *req)
{
    mandala::uid_t uid = req->uid();
    // check duplicates
    for (auto i : _requests) {
        if (i->equals(req)) {
            // the most recent for the uid is the only valid
            if (uid == mandala::cmd::env::nmt::ident::uid)
                return;
            qDebug() << "dup" << Mandala::meta(uid).path;
            delete_request(i);
        }
    }
    _requests.append(req);
    updateProgress();
    emit request_scheduled(req);
}
void PApxNode::reschedule_request(PApxNodeRequest *req)
{
    mandala::uid_t uid = req->uid();
    if (!_requests.contains(req)) {
        qDebug() << "not exists" << Mandala::meta(uid).path;
        delete_request(req);
        return;
    }
    emit request_scheduled(req);
}

void PApxNode::delete_request(PApxNodeRequest *req)
{
    emit request_finished(req);
    _requests.removeAll(req);
    delete req;
    updateProgress();
}
void PApxNode::clear_requests()
{
    for (auto req : _requests) {
        emit request_finished(req);
        delete req;
    }
    _requests.clear();
    updateProgress();
}

void PApxNode::updateProgress()
{
    if (_requests.isEmpty()) {
        setProgress(-1);
        return;
    }
    if (progress() >= 0)
        return;
    setProgress(0);
}

void PApxNode::updateFiles(QStringList fnames)
{
    if (fnames == _files_map.keys()) {
        for (auto i : _files_map) {
            i->reset();
        }
        return;
    }

    for (auto i : _files_map) {
        i->deleteLater();
    }
    _files_map.clear();
    for (auto i : _requests) {
        if (i->uid() == mandala::cmd::env::nmt::file::uid)
            delete_request(i);
    }
    for (auto i : fnames) {
        PApxNodeFile *f = new PApxNodeFile(this, i);
        _files_map.insert(i, f);

        if (i == "dict") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseDictData);
        } else if (i == "conf") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseConfData);
        } else if (i == "script") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseScriptData);
        } else if (i == "mission") {
            connect(f, &PApxNodeFile::downloaded, this, &PApxNode::parseMissionData);
        }
    }
}

bool PApxNode::find_field(QString name,
                          xbus::node::conf::fid_t *fid,
                          xbus::node::conf::type_e *type) const
{
    xbus::node::conf::fid_t v{};
    QRegExp re("_(\\d+)$");
    auto a = re.indexIn(name);
    if (a > 1) {
        name = name.left(a);
        v = re.cap(1).toInt();
        qDebug() << "array" << fid;
    }
    auto i = _field_names.indexOf(name);
    if (i < 0) {
        qWarning() << "missing field:" << name;
        return false;
    }
    v |= i << 8;
    *fid = v;
    *type = _field_types.at(i);
    return true;
}

void PApxNode::parseDictData(const xbus::node::file::info_s &info, const QByteArray data)
{
    PStreamReader stream(data);

    bool err = true;
    QJsonArray fields;
    _field_types.clear();
    _field_names.clear();
    _field_arrays.clear();
    _script_value = {};

    do {
        // check node hash
        if (info.hash != _dict_hash) {
            qWarning() << "node hash error:" << QString::number(info.hash, 16)
                       << QString::number(_dict_hash, 16);
            break;
        }

        QStringList names;
        QList<int> groups;
        QList<int> group_idx;

        while (stream.available() > 4) {
            QJsonObject field;

            xbus::node::conf::type_e type_id = static_cast<xbus::node::conf::type_e>(
                stream.read<uint8_t>());

            QString type = xbus::node::conf::type_to_str(type_id);
            auto array = stream.read<uint8_t>();

            field.insert("type", type);
            field.insert("array", array);

            uint8_t group = stream.read<uint8_t>();

            QStringList st;
            QString name, title;

            bool is_writable = false;

            switch (type_id) {
            case xbus::node::conf::group:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "group" << field.insert(group << st;
                name = st.at(0);
                title = st.at(1);
                group_idx.append(fields.size());
                break;
            case xbus::node::conf::command:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                name = st.at(0);
                title = st.at(1);
                break;
            default:
                st = stream.read_strings(2, stream.available());
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "field" << field.insert(type << field.insert(array << field.insert(group << st;
                name = st.at(0);
                if (!st.at(1).isEmpty())
                    field.insert("units", st.at(1));
                is_writable = true;
            }
            if (name.isEmpty())
                break;

            groups.append(group);
            names.append(name);

            if (title.isEmpty())
                title = name;
            field.insert("title", title);

            // guess path prepended with groups
            QStringList path;
            path.append(name);
            for (auto i = group; i > 0;) {
                i--;
                if (i < group_idx.size()) {
                    int gidx = group_idx.at(i);
                    path.prepend(names.at(gidx));
                    i = groups.at(gidx);
                    continue;
                }
                qWarning() << "missing group:" << type << field.value("array").toInt() << group
                           << st;
                path.clear(); //mark error
                break;
            }
            name = path.join('.');
            field.insert("name", name);

            fields.append(field);

            if (is_writable) {
                _field_types.append(type_id);
                _field_names.append(name);
                _field_arrays.append(array);
            }

            //qDebug() << field << st << stream.available();

            if (stream.available() == 0) {
                err = false;
                break;
            }
        }

    } while (0);

    if (err) {
        qWarning() << "dict error" << data.toHex().toUpper();
        _field_types.clear();
        _field_names.clear();
        _field_arrays.clear();
        _script_value = {};
        return;
    }
    qDebug() << "dict parsed";
    //printf("%s", QJsonDocument(fields).toJson().data());

    emit dictReceived(fields);
}

void PApxNode::parseConfData(const xbus::node::file::info_s &info, const QByteArray data)
{
    PStreamReader stream(data);

    bool err = true;
    QVariantMap values;
    int fidx = 0;

    do {
        // read offsets
        QList<size_t> offsets;
        while (stream.available() >= 2) {
            uint16_t v;
            stream >> v;
            if (!v)
                break;
            offsets.append(v);
        }

        // read Parameters::Data struct
        size_t pos_s = stream.pos();

        // read and check prepended hash
        xbus::node::hash_t hash;
        stream >> hash;

        if (hash != _dict_hash) {
            qWarning() << "data hash error:" << QString::number(hash, 16)
                       << QString::number(_dict_hash, 16);
            break;
        }

        size_t offset_s = 0;
        while (stream.available() > 0) {
            // stream padding with offset
            size_t d_pos = stream.pos() - pos_s;
            size_t offset = offsets.at(fidx);
            size_t d_offset = offset - offset_s;
            offset_s = offset;
            if (d_offset > d_pos) {
                d_pos = d_offset - d_pos;
                //qDebug() << "padding:" << d_pos;
                if (stream.available() < d_pos)
                    break;
                stream.reset(stream.pos() + d_pos);
            } else if (d_offset < d_pos) {
                qWarning() << "padding negative:" << d_offset << d_pos << offsets;
                break;
            }

            pos_s = stream.pos();

            // read value
            auto array = _field_arrays.value(fidx);
            auto type = _field_types.value(fidx);
            QVariant value;

            if (array > 0) {
                QVariantList list;
                for (auto i = 0; i < array; ++i) {
                    QVariant v = read_param(stream, type);
                    if (!v.isValid())
                        break;
                    list.append(v);
                }
                if (list.size() == array)
                    value = QVariant::fromValue(list);
            } else {
                value = read_param(stream, type);
            }

            //qDebug() << v << stream.pos() << stream.available();
            if (!value.isValid())
                break;

            if (type == xbus::node::conf::script) {
                _script_value = value.value<xbus::node::conf::script_t>();
            }

            values.insert(_field_names.value(fidx), value);
            fidx++;

            if (fidx == _field_types.size()) {
                err = false; //stream.available() ? true : false;
                break;
            }
        }

    } while (0);

    if (err) {
        qWarning() << "conf error" << fidx << stream.available() << data.toHex().toUpper();
        return;
    }

    // conf file parsed
    emit confReceived(values);

    if (file("mission"))
        emit missionAvailable();
}
template<typename T, typename Tout = T>
static QVariant _read_param(PStreamReader &stream)
{
    if (stream.available() < sizeof(T))
        return QVariant();
    return QVariant::fromValue(stream.read<T, Tout>());
}
template<typename _T>
static QVariant _read_param_str(PStreamReader &stream)
{
    if (stream.available() < sizeof(_T))
        return QVariant();
    size_t pos_s = stream.pos();
    const char *s = stream.read_string(sizeof(_T));
    if (!s)
        return QVariant();
    stream.reset(pos_s + sizeof(_T));
    return QVariant::fromValue(QString(s));
}

QVariant PApxNode::read_param(PStreamReader &stream, xbus::node::conf::type_e type)
{
    switch (type) {
    case xbus::node::conf::group:
    case xbus::node::conf::command:
    case xbus::node::conf::type_max:
        break;
    case xbus::node::conf::option:
        return ::_read_param<xbus::node::conf::option_t, quint16>(stream);
    case xbus::node::conf::real:
        return ::_read_param<xbus::node::conf::real_t>(stream);
    case xbus::node::conf::byte:
        return ::_read_param<xbus::node::conf::byte_t, quint16>(stream);
    case xbus::node::conf::word:
        return ::_read_param<xbus::node::conf::word_t>(stream);
    case xbus::node::conf::dword:
        return ::_read_param<xbus::node::conf::dword_t>(stream);
    case xbus::node::conf::bind:
        return ::_read_param<xbus::node::conf::bind_t>(stream);
    case xbus::node::conf::string:
        return ::_read_param_str<xbus::node::conf::string_t>(stream);
    case xbus::node::conf::text:
        return ::_read_param_str<xbus::node::conf::text_t>(stream);
    case xbus::node::conf::script:
        return ::_read_param<xbus::node::conf::script_t>(stream);
    }
    return QVariant();
}

template<typename _T>
static bool _write_param(PStreamWriter &stream, QVariant value)
{
    return stream.write<_T>(value.value<_T>());
}

template<typename _T>
static bool _write_param_str(PStreamWriter &stream, QVariant value)
{
    return stream.write_string(value.toString().toLatin1().data());
}

bool PApxNode::write_param(PStreamWriter &stream, xbus::node::conf::type_e type, QVariant value)
{
    switch (type) {
    case xbus::node::conf::group:
    case xbus::node::conf::command:
    case xbus::node::conf::type_max:
        break;
    case xbus::node::conf::option:
        return ::_write_param<xbus::node::conf::option_t>(stream, value);
    case xbus::node::conf::real:
        return ::_write_param<xbus::node::conf::real_t>(stream, value);
    case xbus::node::conf::byte:
        return ::_write_param<xbus::node::conf::byte_t>(stream, value);
    case xbus::node::conf::word:
        return ::_write_param<xbus::node::conf::word_t>(stream, value);
    case xbus::node::conf::dword:
        return ::_write_param<xbus::node::conf::dword_t>(stream, value);
    case xbus::node::conf::bind:
        return ::_write_param<xbus::node::conf::bind_t>(stream, value);
    case xbus::node::conf::string:
        return ::_write_param_str<xbus::node::conf::string_t>(stream, value);
    case xbus::node::conf::text:
        return ::_write_param_str<xbus::node::conf::text_t>(stream, value);
    case xbus::node::conf::script:
        return ::_write_param<xbus::node::conf::script_t>(stream, value);
    }
    return false;
}

void PApxNode::parseScriptData(const xbus::node::file::info_s &info, const QByteArray data)
{
    //qDebug() << "script data" << info.size << data.size();
    // check script hash
    if (info.hash != _script_value) {
        qWarning() << "script hash error:" << QString::number(info.hash, 16)
                   << QString::number(_script_value, 16);
        return;
    }

    if (info.size == 0) {
        // empty script
        emit scriptReceived(QString(), QByteArray(), QByteArray());
        return;
    }

    PStreamReader stream(data);
    xbus::script::file_hdr_s hdr{};
    if (stream.available() < hdr.psize()) {
        qWarning() << "hdr size" << stream.available();
        return;
    }
    hdr.read(&stream);

    if (stream.available() != (hdr.code_size + hdr.src_size)) {
        qWarning() << "size" << stream.available() << hdr.code_size << hdr.src_size;
        return;
    }
    const QByteArray code = stream.toByteArray(stream.pos(), hdr.code_size);
    const QByteArray src = qUncompress(
        stream.toByteArray(stream.pos() + hdr.code_size, hdr.src_size));
    if (src.isEmpty() || code.isEmpty()) {
        qWarning() << "empty" << stream.available() << src.size() << code.size();
        return;
    }
    QString title = QString(QByteArray(hdr.title, sizeof(hdr.title)));

    qDebug() << "script:" << title; // << _script_code.toHex().toUpper();

    emit scriptReceived(title, src, code);
}

void PApxNode::parseMissionData(const xbus::node::file::info_s &info, const QByteArray data)
{
    //qDebug() << "mission data" << info.size << data.size();
    if (data.size() < xbus::mission::file_hdr_s::psize()) {
        emit missionReceived(QJsonValue());
        return;
    }

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
        return;
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
            QString type = "direct";
            if (hdr.option == 1)
                type = "path";
            item.insert("type", type);

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
                qWarning() << "Orphan actions in mission";
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
        return;
    }

    QJsonObject json;
    json.insert("title", title);

    if (!wp.isEmpty())
        json.insert("wp", wp);
    if (!rw.isEmpty())
        json.insert("rw", rw);
    if (!tw.isEmpty())
        json.insert("tw", tw);
    if (!pi.isEmpty())
        json.insert("pi", pi);

    emit missionReceived(json);
}
