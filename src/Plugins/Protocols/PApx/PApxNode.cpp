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

#include <XbusNode.h>
#include <XbusScript.h>

#include <crc.h>

#include <Database/NodesReq.h>
#include <Database/NodesReqDict.h>

PApxNode::PApxNode(PApxNodes *parent, QString uid)
    : PNode(parent, uid)
    , _nodes(parent)
    , _req(this)
{
    // store ident to parse dict
    connect(this, &PNode::identReceived, this, [this](QJsonObject ident) {
        _dict_cache_hash = ident.value("hash").toString();
    });

    // get node info guess from cache
    auto req = new db::nodes::NodeLoadInfo(uid);
    connect(req,
            &db::nodes::NodeLoadInfo::infoLoaded,
            this,
            &PApxNode::infoCacheLoaded,
            Qt::QueuedConnection);
    req->exec();
}

PApxNode::~PApxNode()
{
    _nodes->cancel_requests(this);
}

void PApxNode::infoCacheLoaded(QJsonObject info)
{
    if (!_dict_cache_hash.isEmpty())
        return;
    setTitle(info.value("name").toString());
}

void PApxNode::process_downlink(const xbus::pid_s &pid, PStreamReader &stream)
{
    mandala::uid_t uid = pid.uid;

    if (uid == mandala::cmd::env::nmt::search::uid) {
        if (upgrading())
            return;
        if (pid.pri != xbus::pri_response)
            return;

        requestIdent();
        return;
    }

    //filter zero sized packets if any
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

    // field updates from remote gcs
    if (uid == mandala::cmd::env::nmt::upd::uid) {
        // qDebug() << stream.available();
        if (stream.available() < sizeof(xbus::node::conf::fid_t))
            return;

        auto pos_s = stream.pos();

        xbus::node::conf::fid_t fid;
        stream >> fid;
        const auto fidx = fid >> 8;
        trace()->block(QString::number(fidx));
        trace()->block(QString::number(fid & 0xFF));
        trace()->data(stream.payload());

        if (pid.pri == xbus::pri_request) {
            // field update request from another GCS
            if (fid == 0xFFFFFFFF) { // conf save request
                _ext_upd_request = !_ext_upd_values.isEmpty();
                return;
            }
            _ext_upd_request = false;
            // emit event to update field UI
            const auto name = find_field_name(fid);
            const auto type = _field_types.at(fidx);
            if (name.isEmpty() || name == _script_field)
                return;
            auto value = read_param(stream, type);
            if (value.isNull())
                return;

            if (type == xbus::node::conf::option)
                value = optionToText(value, fidx);
            else if (type == xbus::node::conf::bind)
                value = mandalaToString(value.toInt());

            _ext_upd_values.insert(name, value);
            emit paramsSent({{name, value}});
            return;
        }

        // only responses are processed further
        if (pid.pri != xbus::pri_response) {
            // all other pri types are ignored
            // qDebug() << "wrong pri" << pid.pri;
            return;
        }
        stream.reset(pos_s); // revert to the start of the packet payload

        // intercept conf save event from another GCS
        if (fid == 0xFFFFFFFF && _ext_upd_request) {
            // qDebug() << "conf saved from other GCS";
            emit paramsSaved(_ext_upd_values);
            _ext_upd_values = {};
            _ext_upd_request = false;
        }
    }

    // node messages
    if (uid == mandala::cmd::env::nmt::msg::uid) {
        if (stream.available() < (sizeof(xbus::node::msg::type_e) + 1))
            return;

        xbus::node::msg::type_e t;
        stream >> t;
        trace()->block(QString::number(t));

        bool msg_init = false;

        while (stream.available() > 0) {
            auto p = reinterpret_cast<const char *>(stream.ptr());
            auto sz = stream.available();
            stream.skip(sz);

            // remove trailing zeros
            while (sz > 0 && p[sz - 1] == 0)
                sz--;
            if (!sz)
                continue;

            auto msg = QString::fromUtf8(p, sz);
            msg = msg.replace('\r', '\n').trimmed();

            auto msg_lines = msg.split('\n', Qt::SkipEmptyParts);
            trace()->block(msg_lines.join(" | "));

            for (auto line : msg_lines) {
                line.replace(":", ": ");
                line = line.simplified();
                if (line.isEmpty())
                    continue;

                emit messageReceived((msg_type_e) t, line);

                if (line.contains(QString("%1: initialized").arg(title()), Qt::CaseInsensitive))
                    msg_init = true;
            }
        }

        if (msg_init && !(_nodes->local() || _nodes->upgrading()))
            requestIdent();

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
            if (uid == mandala::cmd::env::nmt::ident::uid || i->active()) {
                delete_request(req);
                return;
            }
            qDebug() << "dup" << req->title();
            delete_request(i);
        }
    }
    _requests.append(req);
    updateProgress();
    emit request_scheduled(req);
}
void PApxNode::reschedule_request(PApxNodeRequest *req)
{
    emit request_scheduled(req);
}
void PApxNode::extend_request(PApxNodeRequest *req, size_t time_ms)
{
    emit request_extended(req, time_ms);
}

void PApxNode::delete_request(PApxNodeRequest *req)
{
    //qDebug() << "finished" << req->title();
    emit req->finished();
    emit request_finished(req);
    delete req;
}
void PApxNode::request_deleted(PApxNodeRequest *req)
{
    _requests.removeOne(req);
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
            connect(f, &PApxNodeFile::uploaded, this, &PApxNode::parseScriptDataUpload);
        }
    }
}

bool PApxNode::find_field(QString name,
                          xbus::node::conf::fid_t *fid,
                          xbus::node::conf::type_e *type) const
{
    xbus::node::conf::fid_t v{};
    static QRegularExpression re("_(\\d+)$");
    auto match = re.match(name);
    auto a = match.capturedStart();
    if (a > 1) {
        auto i = match.captured(1).toInt() - 1;
        if (i < 0) {
            qWarning() << title() << "array" << name;
            return false;
        }
        name = name.left(a);
        v = i;
    }
    auto i = _field_names.indexOf(name);
    if (i < 0) {
        qWarning() << title() << "missing field:" << name;
        return false;
    }
    v |= i << 8;
    *fid = v;
    *type = _field_types.at(i);
    return true;
}
QString PApxNode::find_field_name(xbus::node::conf::fid_t fid) const
{
    auto fidx = fid >> 8;
    if (fidx >= _field_types.size())
        return {};
    auto aidx = fid & 0xFF;
    auto name = _field_names.at(fidx);
    auto array = _field_arrays.at(fidx);
    if (array > 0) {
        if (aidx >= array)
            return {};
        name.append(QString("_%1").arg(aidx + 1));
    }
    return name;
}

QString PApxNode::hashToText(xbus::node::hash_t hash)
{
    return QString("%1").arg(hash, sizeof(hash) * 2, 16, QChar('0')).toUpper();
}

void PApxNode::requestDict()
{
    if (!file("dict"))
        return;

    if (_skip_cache) {
        _skip_cache = false;
        requestDictDownload();
        return;
    }

    auto req = new db::nodes::NodeLoadDict(uid(), _dict_cache_hash);
    connect(req,
            &db::nodes::NodeLoadDict::dictLoaded,
            this,
            &PApxNode::dictCacheLoaded,
            Qt::QueuedConnection);
    connect(req,
            &db::nodes::NodeLoadDict::dictMissing,
            this,
            &PApxNode::dictCacheMissing,
            Qt::QueuedConnection);

    connect(
        req,
        &db::nodes::NodeLoadDict::finished,
        this,
        [this](db::nodes::NodeLoadDict::Status status) {
            if (status)
                requestDictDownload();
        },
        Qt::QueuedConnection);

    req->exec();
}
void PApxNode::dictCacheLoaded(quint64 dictID, QJsonObject dict)
{
    auto hash = dict.value("cache").toString();
    if (_dict_cache_hash != hash) {
        qWarning() << title() << "wrong hash" << _dict_cache_hash << hash;
        return;
    }
    if (dict.isEmpty()) {
        qWarning() << title() << "no dict data";
        requestDictDownload();
        return;
    }

    _field_types.clear();
    _field_names.clear();
    _field_arrays.clear();
    _field_units.clear();

    _rvalues = {};
    _script_hash = {};
    _script_field.clear();

    for (const auto &i : dict.value("fields").toArray()) {
        const auto field = i.toObject();
        auto type = field.value("type").toString();
        xbus::node::conf::type_e type_id{};
        for (uint8_t t = xbus::node::conf::type_field; t < xbus::node::conf::type_max; ++t) {
            QString s = xbus::node::conf::type_to_str((xbus::node::conf::type_e) t);
            if (s != type)
                continue;
            type_id = (xbus::node::conf::type_e) t;
            break;
        }
        if (!type_id)
            continue;
        _field_types.append(type_id);
        _field_names.append(field.value("name").toString());
        _field_arrays.append(field.value("array").toVariant().toUInt());
        _field_units.append(field.value("units").toString());
    }

    qDebug() << title() << "dict from cache" << dictID << _field_names.size();

    emit dictReceived(dict);
}
void PApxNode::dictCacheMissing(QString hash)
{
    if (_dict_cache_hash != hash) {
        qWarning() << title() << "wrong hash" << _dict_cache_hash << hash;
        return;
    }
    requestDictDownload();
}
void PApxNode::parseDictData(PApxNode *node,
                             const xbus::node::file::info_s &info,
                             const QByteArray data)
{
    Q_UNUSED(node)

    PStreamReader stream(data);

    bool err = true;
    QJsonArray fields;
    _field_types.clear();
    _field_names.clear();
    _field_arrays.clear();
    _field_units.clear();

    _rvalues = {};
    _script_hash = {};
    _script_field.clear();

    _ext_upd_values = {};
    _ext_upd_request = false;

    do {
        // check node hash
        QString hash = hashToText(info.hash);
        if (hash != _dict_cache_hash) {
            qWarning() << title() << "node hash error:" << hash << _dict_cache_hash;
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

            if (array > 0)
                field.insert("array", array);

            uint8_t group = stream.read<uint8_t>();

            QStringList st;
            QString name, title, units;

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
                units = st.at(1);
                is_writable = true;
            }
            if (name.isEmpty())
                break;

            groups.append(group);
            names.append(name);

            if (title.isEmpty())
                title = name;
            field.insert("title", title);

            if (units.isEmpty() && type_id == xbus::node::conf::option) {
                units = "off,on";
            }

            if (!units.isEmpty())
                field.insert("units", units);

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
                qWarning() << this->title() << "missing group:" << type << field.value("array")
                           << group << st;
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
                _field_units.append(units);
            }

            //qDebug() << field << st << stream.available();

            if (stream.available() == 0) {
                err = false;
                break;
            }
        }

    } while (0);

    if (err) {
        qWarning() << title() << "dict error" << data.toHex().toUpper();
        _field_types.clear();
        _field_names.clear();
        _field_arrays.clear();
        _field_units.clear();
        return;
    }
    qDebug() << title() << "dict parsed" << _field_names.size();

    QJsonObject dict;
    dict.insert("cache", _dict_cache_hash);
    dict.insert("cached", false);
    dict.insert("fields", fields);

    emit dictReceived(dict);
}

void PApxNode::requestConf()
{
    // save request for sequential script requests
    _req_conf = new PApxNodeRequestFileRead(this, "conf");
    connect(_req_conf, &PApxNodeRequest::finished, this, [this]() { _req_conf = {}; });
}

void PApxNode::parseConfData(PApxNode *node,
                             const xbus::node::file::info_s &info,
                             const QByteArray data)
{
    Q_UNUSED(node)

    PStreamReader stream(data);

    _rvalues = {};
    _script_hash = {};
    _ext_upd_values = {};
    _ext_upd_request = false;

    bool err = true;
    QJsonObject values;
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
        xbus::node::hash_t vhash;
        stream >> vhash;

        auto hash = hashToText(vhash);

        if (hash != _dict_cache_hash) {
            qWarning() << title() << "data hash error:" << hash << _dict_cache_hash;
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
                qWarning() << title() << "padding negative:" << d_offset << d_pos << offsets;
                break;
            }

            pos_s = stream.pos();

            // read value
            auto array = _field_arrays.value(fidx);
            auto type = _field_types.value(fidx);
            auto units = _field_units.value(fidx);
            QJsonValue jsv;

            if (array > 0) {
                QJsonArray list;
                for (auto i = 0; i < array; ++i) {
                    auto jsv_item = read_param(stream, type);
                    if (jsv_item.isNull() || jsv_item.isUndefined())
                        break;
                    if (type == xbus::node::conf::option)
                        jsv_item = optionToText(jsv_item, fidx);
                    else if (type == xbus::node::conf::bind)
                        jsv_item = mandalaToString(jsv_item.toVariant().toUInt());
                    list.append(jsv_item);
                }
                if (list.size() == array)
                    jsv = list;
            } else {
                jsv = read_param(stream, type);
                if (type == xbus::node::conf::option)
                    jsv = optionToText(jsv, fidx);
                else if (type == xbus::node::conf::bind)
                    jsv = mandalaToString(jsv.toVariant().toUInt());
            }

            //qDebug() << v << stream.pos() << stream.available();
            if (jsv.isNull() || jsv.isUndefined())
                break;

            QString name = _field_names.value(fidx);

            if (type == xbus::node::conf::script) {
                _script_hash = jsv.toVariant().value<xbus::node::conf::script_t>();
                _script_field = name;
                jsv = {};
            }

            values.insert(name, jsv);

            fidx++;
            if (fidx == _field_types.size()) {
                err = stream.available() > 8 ? true : false;
                break;
            }
        }

    } while (0);

    if (err) {
        qWarning() << title() << "conf error" << fidx << stream.available()
                   << data.toHex().toUpper();
        _skip_cache = true;
        // requestDict();
        return;
    }

    // conf file parsed
    if (_script_field.isEmpty()) {
        emit confReceived(values);
        return;
    }

    // store values and download script
    _rvalues = values;
    if (_req_conf)
        new PApxNodeRequestFileRead(this, "script");
}
template<typename T, typename Tout = T>
static QJsonValue _read_param(PStreamReader &stream)
{
    if (stream.available() < sizeof(T))
        return {};
    return QJsonValue::fromVariant(QVariant::fromValue(stream.read<T, Tout>()));
}
template<typename _T>
static QJsonValue _read_param_str(PStreamReader &stream)
{
    size_t pos_s = stream.pos();
    const char *s = stream.read_string(sizeof(_T));
    if (!s)
        return {};
    stream.reset(pos_s + sizeof(_T));
    return QString(s);
}

QJsonValue PApxNode::read_param(PStreamReader &stream, xbus::node::conf::type_e type)
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
    return {};
}

template<typename _T>
static bool _write_param(PStreamWriter &stream, QJsonValue value)
{
    return stream.write<_T>(value.toVariant().value<_T>());
}

template<typename _T>
static bool _write_param_str(PStreamWriter &stream, QJsonValue value)
{
    return stream.write_string(value.toString().toLatin1().data());
}

bool PApxNode::write_param(PStreamWriter &stream, xbus::node::conf::type_e type, QJsonValue value)
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

QJsonValue PApxNode::optionToText(const QJsonValue &jsv, size_t fidx)
{
    auto var = jsv.toVariant();
    auto units = _field_units.value(fidx).split(',');
    return units.value(var.toInt(), var.toString());
}
QJsonValue PApxNode::textToOption(const QJsonValue &jsv, size_t fidx)
{
    auto s = jsv.toVariant().toString();
    auto units = _field_units.value(fidx).split(',');
    if (units.contains(s)) {
        xbus::node::conf::option_t opt = units.indexOf(s);
        return opt;
    }
    return jsv;
}

void PApxNode::parseScriptDataUpload(PApxNode *node,
                                     const xbus::node::file::info_s &info,
                                     const QByteArray data)
{
    _script_hash = info.hash;
    parseScriptData(node, info, data);
}
void PApxNode::parseScriptData(PApxNode *node,
                               const xbus::node::file::info_s &info,
                               const QByteArray data)
{
    Q_UNUSED(node)

    //qDebug() << "script data" << info.size << data.size();
    // check script hash
    if (info.hash != _script_hash) {
        qWarning() << title() << "script hash error:" << QString::number(info.hash, 16)
                   << QString::number(_script_hash, 16);
        return;
    }

    if (_script_field.isEmpty()) {
        qWarning() << title() << "missing script field";
        return;
    }

    QJsonValue value;
    if (info.size > 0) {
        PStreamReader stream(data);
        xbus::script::file_hdr_s hdr{};
        if (stream.available() < hdr.psize()) {
            qWarning() << title() << "hdr size" << stream.available();
            return;
        }
        hdr.read(&stream);

        if (stream.available() != (hdr.code_size + hdr.src_size)) {
            qWarning() << title() << "size" << stream.available() << hdr.code_size << hdr.src_size;
            return;
        }
        const QByteArray code = stream.toByteArray(stream.pos(), hdr.code_size);
        const QByteArray src = qUncompress(
            stream.toByteArray(stream.pos() + hdr.code_size, hdr.src_size));
        if (src.isEmpty() || code.isEmpty()) {
            qWarning() << title() << "empty" << stream.available() << src.size() << code.size();
            return;
        }
        auto scr_title = QString::fromUtf8(hdr.title, strnlen(hdr.title, sizeof(hdr.title)));

        qDebug() << title() << "script:" << scr_title; // << _script_code.toHex().toUpper();

        QStringList st;
        st << scr_title;
        st << qCompress(src, 9).toHex().toUpper();
        st << qCompress(code, 9).toHex().toUpper();
        value = st.join(',');
    }

    // append script data to values
    _rvalues.insert(_script_field, value);
    emit confReceived(_rvalues);
    _rvalues = {};
}

void PApxNode::requestUpdate(QJsonObject values)
{
    if (values.contains(_script_field)) {
        // script data must be decoded to file data and field value (hash)
        _script_wdata = values.value(_script_field);
        QByteArray data = pack_script(_script_wdata);
        if (data.isEmpty()) {
            _script_hash = {};
            qDebug() << title() << "empty script";
        } else {
            _script_hash = apx::crc32(data.data(), data.size());
        }
        values.insert(_script_field, (qint64) _script_hash);
        new PApxNodeRequestFileWrite(this, "script", data);
    }

    auto req = new PApxNodeRequestUpdate(this, values);
    connect(req, &PApxNodeRequestUpdate::sent, this, [this](QString name, QJsonValue value) {
        if (name == _script_field)
            value = _script_wdata;
        emit paramsSent({{name, value}});
    });
    connect(req, &PApxNodeRequestUpdate::saved, this, [this](QJsonObject values) {
        if (values.contains(_script_field))
            values.insert(_script_field, _script_wdata);
        emit paramsSaved(values);
    });
}
QByteArray PApxNode::pack_script(const QJsonValue &jsv)
{
    QStringList st = jsv.toString().split(',', Qt::KeepEmptyParts);

    if (st.size() != 3)
        return {};

    QString title = st.at(0);
    QByteArray src = QByteArray::fromHex(st.at(1).toLocal8Bit());
    QByteArray code = qUncompress(QByteArray::fromHex(st.at(2).toLocal8Bit()));

    QByteArray data(xbus::script::max_file_size, '\0');
    PStreamWriter stream(data.data(), data.size());

    xbus::script::file_hdr_s hdr{};
    hdr.code_size = code.size();
    hdr.src_size = src.size();
    strncpy(hdr.title, title.toUtf8(), sizeof(xbus::script::title_t));
    hdr.write(&stream);
    stream.append(code);
    stream.append(src);
    return stream.toByteArray();
}
