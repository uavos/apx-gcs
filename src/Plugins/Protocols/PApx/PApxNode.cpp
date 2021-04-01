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

#include <xbus/XbusNode.h>
#include <xbus/XbusScript.h>

#include <crc.h>

PApxNode::PApxNode(PApxNodes *parent, QString uid)
    : PNode(parent, uid)
    , _nodes(parent)
    , _req(this)
{
    // store ident to parse dict
    connect(this, &PNode::identReceived, this, [this](QVariantMap ident) {
        _dict_hash = ident.value("hash").value<xbus::node::hash_t>();
    });
}

PApxNode::~PApxNode()
{
    _nodes->cancel_requests(this);
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

    // field updates from remote gcs
    if (uid == mandala::cmd::env::nmt::upd::uid && pid.pri == xbus::pri_request) {
        if (stream.available() != sizeof(xbus::node::conf::fid_t))
            return;

        xbus::node::conf::fid_t fid;
        stream >> fid;
        trace()->block(QString::number(fid >> 8));
        trace()->block(QString::number(fid & 0xFF));
        trace()->data(stream.payload());
        size_t fidx = fid >> 8;
        if (fidx >= _field_types.size())
            return;
        QString name = _field_names.at(fidx);
        QVariant value = read_param(stream, _field_types.at(fidx));
        QVariantMap values;
        values.insert(name, value);
        emit confUpdated(values);
        return;
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
    emit request_scheduled(req);
}
void PApxNode::extend_request(PApxNodeRequest *req, size_t time_ms)
{
    emit request_extended(req, time_ms);
}

void PApxNode::delete_request(PApxNodeRequest *req)
{
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
        }
    }
}
void PApxNode::file_uploaded(QString name)
{
    if (name == "mission") {
        findParent<PApxVehicle>()->mission()->missionUpdated();
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
    QVariantList fields;
    _field_types.clear();
    _field_names.clear();
    _field_arrays.clear();

    _values.clear();
    _script_value = {};
    _script_field.clear();

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
            QVariantMap field;

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
        return;
    }
    qDebug() << "dict parsed";

    emit dictReceived(fields);
}

void PApxNode::parseConfData(const xbus::node::file::info_s &info, const QByteArray data)
{
    PStreamReader stream(data);

    _values.clear();

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

            QString name = _field_names.value(fidx);

            if (type == xbus::node::conf::script) {
                _script_value = value.value<xbus::node::conf::script_t>();
                _script_field = name;
                value = QVariant();
            }

            values.insert(name, value);

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
    if (_script_field.isEmpty()) {
        emit confReceived(values);
        return;
    }

    // store values and download script
    _values = values;
    new PApxNodeRequestFileRead(this, "script");
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

    if (_script_field.isEmpty()) {
        qWarning() << "missing script field";
        return;
    }

    QVariant value;
    if (info.size > 0) {
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

        QStringList st;
        st << title;
        st << qCompress(src, 9).toHex().toUpper();
        st << qCompress(code, 9).toHex().toUpper();
        value = st.join(',');
    }

    _values.insert(_script_field, value);
    emit confReceived(_values);
    _values.clear();
}

void PApxNode::requestUpdate(QVariantMap values)
{
    if (values.contains(_script_field)) {
        QByteArray data = pack_script(values.value(_script_field));
        if (data.isEmpty()) {
            _script_value = {};
            qDebug() << "empty script";
        } else {
            _script_value = apx::crc32(data.data(), data.size());
        }
        values.insert(_script_field, _script_value);
        new PApxNodeRequestFileWrite(this, "script", data);
    }

    new PApxNodeRequestUpdate(this, values);
}
QByteArray PApxNode::pack_script(QVariant value)
{
    QStringList st = value.toString().split(',', Qt::KeepEmptyParts);

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
