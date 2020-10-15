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
#include "ProtocolNode.h"
#include "ProtocolNodes.h"
#include "ProtocolVehicle.h"
#include "ProtocolVehicles.h"

#include <App/App.h>
#include <Mandala/Mandala.h>

#include <Xbus/XbusNode.h>
#include <Xbus/XbusScript.h>

ProtocolNode::ProtocolNode(ProtocolNodes *nodes, const QString &sn)
    : ProtocolBase(nodes, "node")
    , _nodes(nodes)
    , m_sn(sn)
{
    setIcon("sitemap");

    memset(&m_ident, 0, sizeof(m_ident));

    connect(nodes, &ProtocolNodes::activeChanged, this, [this, nodes]() {
        if (!nodes->active()) {
            setProgress(-1);
            setValue(QVariant());
        }
    });

    connect(this,
            &ProtocolNode::hardwareChanged,
            this,
            &ProtocolNode::updateDescr,
            Qt::QueuedConnection);
    connect(this,
            &ProtocolNode::versionChanged,
            this,
            &ProtocolNode::updateDescr,
            Qt::QueuedConnection);
    connect(this,
            &ProtocolNode::enabledChanged,
            this,
            &ProtocolNode::updateDescr,
            Qt::QueuedConnection);
    connect(this,
            &ProtocolNode::identChanged,
            this,
            &ProtocolNode::updateDescr,
            Qt::QueuedConnection);
    connect(this,
            &ProtocolNode::filesChanged,
            this,
            &ProtocolNode::updateDescr,
            Qt::QueuedConnection);

    connect(this, &ProtocolNode::confReceived, this, [this](const QVariantMap &values) {
        _values = values;
    });

    if (vehicle()->isReplay())
        return;

    connect(this, &ProtocolNode::confSaved, this, [this]() {
        vehicle()->storage->saveNodeConfig(this);
    });

    vehicle()->storage->loadNodeInfo(this);
    requestIdent();
}

ProtocolVehicle *ProtocolNode::vehicle() const
{
    return _nodes->vehicle();
}

void ProtocolNode::updateDescr()
{
    QStringList st;
    st.append(hardware());
    if (version() != App::version())
        st.append(version());
    if (!enabled())
        st.append(tr("offline"));
    if (ident().flags.bits.files == 1 && file("fw"))
        st.append("LOADER");
    //st.append(sn());
    setDescr(st.join(' '));
}
QString ProtocolNode::toolTip() const
{
    QStringList st;
    st.append(QString("sn: %1").arg(sn()));
    st.append(QString("files: %1").arg(files().join(',')));
    st.append(QString("version: %1").arg(version()));
    st.append(QString("hardware: %1").arg(hardware()));
    st.append(QString("fields: %1").arg(m_dict_fields.size()));
    return ProtocolBase::toolTip().append("\n").append(st.join('\n'));
}
void ProtocolNode::hashData(QCryptographicHash *h) const
{
    ProtocolBase::hashData(h);
    h->addData(version().toUtf8());
    h->addData(hardware().toUtf8());
    h->addData(QString::number(ident().hash).toUtf8());
}

void ProtocolNode::dbDictInfoFound(QVariantMap dictInfo)
{
    //qDebug() << vehicle()->title() << title() << dictInfo;
    m_dbDictInfo = dictInfo;
}
void ProtocolNode::dbConfigIDFound(quint64 configID)
{
    m_dbConfigID = configID;
}
void ProtocolNode::setDict(const ProtocolNode::Dict &dict)
{
    m_dict = dict;
    m_dict_fields.clear();
    for (auto i = 0; i < m_dict.size(); ++i) {
        auto const &f = m_dict.at(i);
        if (f.type >= xbus::node::conf::type_field)
            m_dict_fields.append(i);
    }
    setDictValid(true);
    emit dictReceived(m_dict);
}

void ProtocolNode::downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    //filter requests
    while (stream.available() == 0) {
        if (pid.uid == mandala::cmd::env::nmt::search::uid)
            break;
        return;
    }

    m_lastSeenTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

    //qDebug() << QString("[%1]").arg(Mandala::meta(pid).name) << stream.available();

    switch (pid.uid) {
    default:
        //qDebug() << cmd << data.size();
        trace_downlink(stream.payload());
        return;

    case mandala::cmd::env::nmt::search::uid: { //response to search
        //qDebug() << "apc_search" << sn;
        trace_downlink(stream.payload());
        requestIdent();
    } break;

        // node ident
    case mandala::cmd::env::nmt::ident::uid: {
        //qDebug() << "ident" << sn;
        trace_downlink(stream.payload());
        if (stream.available() <= (xbus::node::ident::ident_s::psize() + 3 * 2)) {
            qWarning() << "size" << stream.available() << xbus::node::ident::ident_s::psize();
            break;
        }
        _nodes->acknowledgeRequest(pid, stream);
        xbus::node::ident::ident_s ident;
        ident.read(&stream);

        QStringList st = stream.read_strings(3);
        if (st.isEmpty())
            break;
        QString sname = st.at(0);
        QString sversion = st.at(1);
        QString shardware = st.at(2);

        QStringList fnames = stream.read_strings(ident.flags.bits.files);
        for (auto i : fnames) {
            if (!i.isEmpty())
                continue;
            fnames.clear();
            break;
        }
        if (fnames.isEmpty())
            break;

        if (stream.available() > 0)
            break;

        fnames.sort();

        setName(sname.toLower());
        setTitle(sname);
        setVersion(sversion);
        setHardware(shardware);

        if (setIdent(ident) || fnames != files()) {
            resetFilesMap();
            setFiles(fnames);
        }
        emit identReceived();
        if (ident.flags.bits.files > 1) {
            vehicle()->storage->saveNodeInfo(this);
            vehicle()->storage->saveNodeUser(this);
        }
        _nodes->nodeNotify(this);

        // continue requests
        if (!dictValid()) {
            vehicle()->storage->loadNodeDict(this);
        } else if (!valid()) {
            requestConf();
        }

    } break;

        // file operations
    case mandala::cmd::env::nmt::file::uid: {
        if (vehicle()->isLocal() && !_nodes->active())
            return;

        if (stream.available() <= sizeof(xbus::node::file::op_e))
            break;
        xbus::node::file::op_e op;
        stream >> op;

        if (!(op & xbus::node::file::reply_op_mask))
            break;
        op = static_cast<xbus::node::file::op_e>(op & ~xbus::node::file::reply_op_mask);
        trace_downlink(QString::number(op));

        const char *s = stream.read_string(16);
        if (!s)
            break;
        trace_downlink(QString(s));
        ProtocolNodeFile *f = file(s);
        if (!f)
            break;
        trace_downlink(stream.payload());
        f->downlink(op, stream);
    } break;

        // message from vehicle
    case mandala::cmd::env::nmt::msg::uid: {
        if (stream.available() < (sizeof(xbus::node::msg::type_e) + 1))
            break;

        xbus::node::msg::type_e t;
        stream >> t;
        trace_downlink(QString::number(t));

        const char *s = stream.read_string(stream.available());
        QString msg(QString(s).trimmed());
        trace_downlink(msg);

        if (msg.isEmpty())
            break;

        msg.replace(":", ": ");
        msg = msg.simplified();

        emit messageReceived(t, msg);
    } break;

        // reboot ack
    case mandala::cmd::env::nmt::reboot::uid: {
        trace_downlink(stream.payload());
        if (stream.available() != sizeof(xbus::node::reboot::type_e))
            break;
        stream.discard();
        _nodes->acknowledgeRequest(pid, stream);
    } break;

        // upd ack
    case mandala::cmd::env::nmt::upd::uid: {
        trace_downlink(stream.payload());
        if (stream.available() != sizeof(xbus::node::conf::fid_t))
            break;
        stream.discard();
        _nodes->acknowledgeRequest(pid, stream);
    } break;

        // usr ack
    case mandala::cmd::env::nmt::usr::uid: {
        trace_downlink(stream.payload());
        if (stream.available() != sizeof(xbus::node::usr::cmd_t))
            break;
        stream.discard();
        _nodes->acknowledgeRequest(pid, stream);
    } break;

        // status from node
        /*case mandala::cmd::env::nmt::status::uid: {
        if (stream.available() != sizeof(xbus::node::status::status_s))
            break;
        nodes->acknowledgeRequest(m_sn, pid);

        trace_downlink(stream.payload());

        xbus::node::status::status_s status;
        status.read(&stream);

        emit statusReceived(status);
    } break;*/
    }
}

void ProtocolNode::resetFilesMap()
{
    qDeleteAll(_files_map.values());
    _files_map.clear();
}
ProtocolNodeFile *ProtocolNode::file(const QString &fname)
{
    if (!files().contains(fname))
        return nullptr;
    ProtocolNodeFile *f = _files_map.value(fname, nullptr);
    if (f)
        return f;
    f = new ProtocolNodeFile(this, fname);
    _files_map.insert(fname, f);
    connect(_nodes, &Fact::activeChanged, f, [f]() { f->stop(); });
    return f;
}

ProtocolNodeRequest *ProtocolNode::request(mandala::uid_t uid, size_t retry_cnt)
{
    return _nodes->request(uid, m_sn, retry_cnt);
}

void ProtocolNode::requestReboot()
{
    _nodes->clear_requests();
    _nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::reboot::uid);
    req->write<xbus::node::reboot::type_e>(xbus::node::reboot::firmware);
    req->schedule();
}
void ProtocolNode::requestRebootLoader()
{
    setIdentValid(false);
    _nodes->clear_requests();
    _nodes->setActive(true);
    setProgress(0);
    timeReqLoader.start();
    requestRebootLoaderNext();
}
void ProtocolNode::requestRebootLoaderNext()
{
    qDebug() << "ldr";
    if (timeReqLoader.elapsed() > 10000) {
        qWarning() << "timeout";
        _nodes->setActive(false);
        return;
    }
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::reboot::uid, 0);
    req->write<xbus::node::reboot::type_e>(xbus::node::reboot::loader);
    connect(
        req,
        &ProtocolNodeRequest::finished,
        this,
        [this]() {
            ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::ident::uid, 0);
            connect(
                req,
                &ProtocolNodeRequest::finished,
                this,
                [this]() {
                    if (!file("fw") && _nodes->active())
                        requestRebootLoaderNext();
                },
                Qt::QueuedConnection);
            req->schedule();
        },
        Qt::QueuedConnection);
    req->schedule();
}

void ProtocolNode::requestIdent()
{
    if (vehicle()->isLocal() && !_nodes->active())
        return;

    request(mandala::cmd::env::nmt::ident::uid)->schedule();
}
void ProtocolNode::requestDict()
{
    if (vehicle()->isLocal() && !_nodes->active())
        return;

    if (dictValid()) {
        requestConf();
        return;
    }

    ProtocolNodeFile *f = file("dict");
    if (!f) {
        qWarning() << "Dict unavailable";
        return;
    }
    f->stop();
    connect(f,
            &ProtocolNodeFile::downloaded,
            this,
            &ProtocolNode::parseDictData,
            Qt::UniqueConnection);
    f->download();
}
void ProtocolNode::requestConf()
{
    if (vehicle()->isLocal() && !_nodes->active())
        return;

    if (valid()) {
        return;
    }

    ProtocolNodeFile *f = file("conf");
    if (!f) {
        qWarning() << "Conf unavailable";
        return;
    }
    f->stop();
    connect(f,
            &ProtocolNodeFile::downloaded,
            this,
            &ProtocolNode::parseConfData,
            Qt::UniqueConnection);
    f->download();
}
void ProtocolNode::requestStatus()
{
    /*nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::status::uid);
    *req << type;
    req->schedule();*/
}

void ProtocolNode::requestUpdate(const QVariantMap &values)
{
    _update_requests.clear();
    _nodes->clear_requests();
    for (auto const &fpath : values.keys()) {
        const dict_field_s *f = field(fpath);
        if (!f) {
            qWarning() << "missing field" << fpath;
            continue;
        }
        QVariant value = values.value(fpath);
        if (f->type == xbus::node::conf::option) {
            _values[fpath] = f->units.split(',').value(value.toInt(), value.toString());
        } else if (f->type == xbus::node::conf::real) {
            _values[fpath] = value.toString();
        } else if (f->type == xbus::node::conf::script) {
            if (fpath != _script_fpath) {
                qWarning() << "script field mismatch:" << fpath << _script_fpath;
                return;
            }
            QByteArray data = scriptFileData(value);
            _script_hash = apx::crc32(data.data(), data.size());
            qDebug() << "script:" << data.size();
            _parseScript(data);

            ProtocolNodeFile *f = file("script");
            if (!f) {
                qWarning() << "Script unavailable";
                return;
            }
            connect(f,
                    &ProtocolNodeFile::uploaded,
                    this,
                    &ProtocolNode::updateRequestsCheckDone,
                    Qt::UniqueConnection);
            _update_requests.append(f);
            f->stop();
            _nodes->setActive(true);
            f->upload(data);
            value = QVariant::fromValue(_script_hash);
        } else {
            _values[fpath] = value;
        }

        // request
        _nodes->setActive(true);
        ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::upd::uid);
        xbus::node::conf::fid_t id = fid(fpath);
        *req << id;
        if (!write_param(*req, id, value)) {
            req->deleteLater();
            _nodes->clear_requests();
            qWarning() << "update request error:" << fpath;
            return;
        }
        connect(req, &ProtocolNodeRequest::finished, this, &ProtocolNode::updateRequestsCheckDone);
        _update_requests.append(req);
        connect(req, &ProtocolNodeRequest::timeout, _nodes, &ProtocolNodes::clear_requests);
        req->schedule();
    }
    if (_update_requests.isEmpty()) {
        qWarning() << "nothing to update";
        return;
    }
}
void ProtocolNode::updateRequestsCheckDone()
{
    if (!_update_requests.contains(sender()))
        return;
    ProtocolNodeRequest *sreq = qobject_cast<ProtocolNodeRequest *>(sender());
    if (sreq && !sreq->acknowledged)
        return;
    _update_requests.removeAll(sender());
    if (!_update_requests.isEmpty())
        return;
    // save to node storage request
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::upd::uid);
    req->write<xbus::node::conf::fid_t>(0xFFFF);
    connect(req, &ProtocolNodeRequest::finished, this, [this](ProtocolNodeRequest *request) {
        if (request->acknowledged) {
            emit confSaved();
        } else {
            _nodes->clear_requests();
        }
    });
    req->schedule();
}

void ProtocolNode::requestMod(QStringList commands)
{
    _nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::mod::uid, 0);
    xbus::node::mod::op_e op = xbus::node::mod::sh;
    *req << op;
    for (auto const &s : commands) {
        req->write_string(s.toUtf8().data());
    }
    req->schedule();
}
void ProtocolNode::requestUsr(xbus::node::usr::cmd_t cmd, QByteArray data)
{
    _nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::usr::uid);
    *req << cmd;
    req->append(data);
    req->schedule();
}

void ProtocolNode::parseDictData(const xbus::node::file::info_s &info, const QByteArray data)
{
    ProtocolStreamReader stream(data);
    bool err = true;
    m_dict.clear();
    m_dict_fields.clear();
    QList<int> groups;
    Dict dict;
    do {
        // check node hash
        if (info.hash != m_ident.hash) {
            qWarning() << "node hash error:" << QString::number(info.hash, 16)
                       << QString::number(m_ident.hash, 16);
            break;
        }

        while (stream.available() > 4) {
            dict_field_s field;

            field.type = static_cast<xbus::node::conf::type_e>(stream.read<uint8_t>());
            field.array = stream.read<uint8_t>();
            field.group = stream.read<uint8_t>();

            QStringList st;

            switch (field.type) {
            case xbus::node::conf::group:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "group" << field.group << st;
                field.name = st.at(0);
                field.title = st.at(1);
                groups.append(dict.size());
                break;
            case xbus::node::conf::command:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                field.name = st.at(0);
                field.title = st.at(1);
                break;
            default:
                st = stream.read_strings(2, stream.available());
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "field" << field.type << field.array << field.group << st;
                field.name = st.at(0);
                field.title = st.at(0);
                field.units = st.at(1);
            }
            if (field.name.isEmpty())
                break;

            if (field.title.isEmpty())
                field.title = field.name;

            // guess path prepended with groups
            QStringList path;
            path.append(field.name);
            uint8_t group = field.group;
            while (group) {
                group--;
                if (group < groups.size()) {
                    const dict_field_s &g = dict.at(groups.at(group));
                    path.prepend(g.name);
                    group = g.group;
                    continue;
                }
                qWarning() << "missing group:" << field.type << field.array << field.group << st;
                path.clear(); //mark error
                break;
            }
            field.path = path.join('.');

            // push to dict
            dict.append(field);

            //qDebug() << field.name << field.type << field.array << field.group << st
            //         << stream.available();

            if (stream.available() == 0) {
                err = false;
                break;
            }
        }

    } while (0);

    if (err) {
        setDictValid(false);
        qWarning() << "dict error" << data.toHex().toUpper();
        return;
    }
    qDebug() << "dict parsed";
    setDict(dict);

    vehicle()->storage->saveNodeDict(this, m_dict);

    requestConf();
}

void ProtocolNode::parseConfData(const xbus::node::file::info_s &info, const QByteArray data)
{
    ProtocolStreamReader stream(data);
    bool err = true;
    QVariantList values;
    xbus::node::conf::fid_t fid = 0;
    do {
        // check node hash
        if (info.hash != m_ident.hash) {
            qWarning() << "file hash error:" << QString::number(info.hash, 16)
                       << QString::number(m_ident.hash, 16);
            break;
        }

        // read offsets
        QList<size_t> offsets;
        while (stream.available() >= 2) {
            uint16_t v;
            stream >> v;
            if (!v)
                break;
            offsets.append(v);
        }
        if (offsets.size() != m_dict_fields.size()) {
            qWarning() << "offsets size:" << offsets.size() << m_dict_fields.size() << offsets;
            break;
        }

        // read Parameters::Data struct
        size_t pos_s = stream.pos();

        // read and check prepended hash
        xbus::node::hash_t dhash;
        stream >> dhash;
        if (info.hash != m_ident.hash) {
            qWarning() << "data hash error:" << QString::number(dhash, 16)
                       << QString::number(m_ident.hash, 16);
            break;
        }

        size_t offset_s = 0;
        while (stream.available() > 0) {
            // stream padding with offset
            size_t d_pos = stream.pos() - pos_s;
            size_t offset = offsets.at(fid);
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
            QVariant v = read_param(stream, fid);
            //qDebug() << v << stream.pos() << stream.available();
            if (!v.isValid())
                break;

            // check dict and fix field value for some types
            const dict_field_s &f = m_dict.at(m_dict_fields.at(fid));
            if (f.type == xbus::node::conf::option) {
                const QStringList st = f.units.split(',');
                if (f.array > 0) {
                    QVariantList list;
                    for (auto const &i : v.value<QVariantList>())
                        list.append(st.value(i.toInt(), i.toString()));
                    v = QVariant::fromValue<QVariantList>(list);
                } else {
                    v = st.value(v.toInt(), v.toString());
                }
            } else if (f.type == xbus::node::conf::real) {
                if (f.array > 0) {
                    QVariantList list;
                    for (auto const &i : v.value<QVariantList>())
                        list.append(QString::number(i.toFloat()));
                    v = QVariant::fromValue<QVariantList>(list);
                } else {
                    v = QString::number(v.toFloat());
                }
            }

            values.append(v);
            fid++;

            if (fid == m_dict_fields.size()) {
                err = false; //stream.available() ? true : false;
                break;
            }
        }

    } while (0);

    // conf file parsed
    do {
        if (err)
            break;

        // collect values
        _script_fpath.clear();
        _values.clear();
        for (auto i = 0; i < values.size(); ++i) {
            const dict_field_s &f = m_dict.at(m_dict_fields.at(i));
            if (f.type == xbus::node::conf::script) {
                _script_fpath = f.path;
                _script_hash = values.at(i).value<xbus::node::hash_t>();
                continue;
            }
            _values.insert(f.path, values.at(i));
        }
        qDebug() << "conf parsed";

        if (!_script_fpath.isEmpty()) {
            qDebug() << "script field" << _script_fpath;
            ProtocolNodeFile *f = file("script");
            if (!f) {
                qWarning() << "Script unavailable";
                break;
            }
            f->stop();
            connect(f,
                    &ProtocolNodeFile::downloaded,
                    this,
                    &ProtocolNode::parseScriptData,
                    Qt::UniqueConnection);
            f->download();
            return;
        }

        validate();
        return;
    } while (0);

    // error
    _script_fpath.clear();
    setValid(false);
    qWarning() << "conf error" << fid << stream.available() << data.toHex().toUpper();
}
void ProtocolNode::validate()
{
    // notify
    emit confReceived(_values);
    setValid(true);

    if (ident().flags.bits.reconf) {
        emit confDefault();
        vehicle()->storage->loadNodeConfig(this);
        return;
    }

    // save config
    vehicle()->storage->saveNodeConfig(this);
}
void ProtocolNode::parseScriptData(const xbus::node::file::info_s &info, const QByteArray data)
{
    qDebug() << "script data" << info.size << data.size();
    do {
        if (_script_fpath.isEmpty()) {
            qWarning() << "script idx err" << data.size() << data.toHex().toUpper();
            setValid(false);
            _resetScript();
            return;
        }

        // check node hash
        if (info.hash != _script_hash) {
            qWarning() << "file hash error:" << QString::number(info.hash, 16)
                       << QString::number(_script_hash, 16);
            break;
        }

        if (info.size == 0) {
            // empty script
            break;
        }

        if (!_parseScript(data))
            break;

        validate();
        return;

    } while (0);

    // error
    _resetScript();
    validate();
    //setValid(false);
    qWarning() << "script error" << data.size(); // << data.toHex().toUpper();
}
bool ProtocolNode::_parseScript(const QByteArray data)
{
    do {
        if (!field(_script_fpath)) {
            qWarning() << "script field missing" << _script_fpath;
            break;
        }
        _values.insert(_script_fpath, QString());

        ProtocolStreamReader stream(data);
        xbus::script::file_hdr_s hdr{};
        if (stream.available() < hdr.psize()) {
            qWarning() << "hdr size" << stream.available();
            break;
        }
        hdr.read(&stream);

        if (stream.available() != (hdr.code_size + hdr.src_size)) {
            qWarning() << "size" << stream.available() << hdr.code_size << hdr.src_size;
            break;
        }
        const QByteArray code = stream.toByteArray(stream.pos(), hdr.code_size);
        const QByteArray src = qUncompress(
            stream.toByteArray(stream.pos() + hdr.code_size, hdr.src_size));
        if (src.isEmpty() || code.isEmpty()) {
            qWarning() << "empty" << stream.available() << src.size() << code.size();
            break;
        }
        QString title = QString(QByteArray(hdr.title, sizeof(hdr.title)));

        QStringList st;
        st << title;
        st << qCompress(src, 9).toHex().toUpper();
        st << qCompress(code, 9).toHex().toUpper();
        _values.insert(_script_fpath, st.join(','));

        qDebug() << "script:" << title; // << _script_code.toHex().toUpper();
        return true;
    } while (0);
    _resetScript();
    return false;
}
void ProtocolNode::_resetScript()
{
    if (field(_script_fpath))
        _values.insert(_script_fpath, QString());
}

QByteArray ProtocolNode::scriptFileData(const QVariant &value) const
{
    QStringList st = value.toString().split(',', Qt::KeepEmptyParts);
    QString title;
    QByteArray src;
    QByteArray code;
    if (st.size() != 3)
        return QByteArray();

    title = st.at(0);
    src = QByteArray::fromHex(st.at(1).toLocal8Bit());
    code = qUncompress(QByteArray::fromHex(st.at(2).toLocal8Bit()));

    QByteArray ba(xbus::script::max_file_size, '\0');
    ProtocolStreamWriter stream(ba.data(), ba.size());

    xbus::script::file_hdr_s hdr{};
    strncpy(hdr.title, title.toLocal8Bit(), sizeof(hdr.title));

    hdr.code_size = code.size();
    hdr.src_size = src.size();

    hdr.write(&stream);
    stream.append(code);
    stream.append(src);

    return ba.left(stream.pos());
}

const ProtocolNode::dict_field_s *ProtocolNode::field(xbus::node::conf::fid_t fid) const
{
    if (fid >= m_dict_fields.size())
        return nullptr;
    return &m_dict.at(m_dict_fields.at(fid));
}
const ProtocolNode::dict_field_s *ProtocolNode::field(const QString &fpath) const
{
    if (fpath.isEmpty())
        return nullptr;
    for (auto i : m_dict_fields) {
        if (m_dict.at(i).path == fpath)
            return &m_dict.at(i);
    }
    return nullptr;
}
xbus::node::conf::fid_t ProtocolNode::fid(const QString &fpath) const
{
    xbus::node::conf::fid_t fid{0};
    for (auto i : m_dict_fields) {
        if (m_dict.at(i).path == fpath)
            return fid;
        fid++;
    }
    return 0;
}

template<typename T, typename Tout = T>
static QVariant read_param(ProtocolStreamReader &stream, size_t array)
{
    if (stream.available() < (sizeof(T) * (array ? array : 1)))
        return QVariant();
    if (array > 0) {
        QVariantList list;
        while (array--) {
            list.append(QVariant::fromValue(stream.read<T, Tout>()));
        }
        //qDebug() << list;
        return QVariant::fromValue(list);
    }
    return QVariant::fromValue(stream.read<T, Tout>());
}
template<typename _T>
static QVariant read_param_str(ProtocolStreamReader &stream, size_t array)
{
    if (stream.available() < (sizeof(_T) * (array ? array : 1)))
        return QVariant();
    if (array > 0) {
        QVariantList list;
        while (array--) {
            size_t pos_s = stream.pos();
            const char *s = stream.read_string(sizeof(_T));
            if (!s)
                return QVariant();
            stream.reset(pos_s + sizeof(_T));
            list.append(QVariant::fromValue(QString(s)));
        }
        return QVariant::fromValue(list);
    }
    size_t pos_s = stream.pos();
    const char *s = stream.read_string(sizeof(_T));
    if (!s)
        return QVariant();
    stream.reset(pos_s + sizeof(_T));
    return QVariant::fromValue(QString(s));
}

QVariant ProtocolNode::read_param(ProtocolStreamReader &stream, xbus::node::conf::fid_t fid)
{
    const dict_field_s *f = field(fid);
    if (!f)
        return QVariant();
    //qDebug() << f->name << stream.payload().toHex().toUpper();
    size_t array = f->array;
    switch (f->type) {
    case xbus::node::conf::group:
    case xbus::node::conf::command:
    case xbus::node::conf::type_max:
        break;
    case xbus::node::conf::option:
        return ::read_param<xbus::node::conf::option_t, quint16>(stream, array);
    case xbus::node::conf::real:
        return ::read_param<xbus::node::conf::real_t>(stream, array);
    case xbus::node::conf::byte:
        return ::read_param<xbus::node::conf::byte_t, quint16>(stream, array);
    case xbus::node::conf::word:
        return ::read_param<xbus::node::conf::word_t>(stream, array);
    case xbus::node::conf::dword:
        return ::read_param<xbus::node::conf::dword_t>(stream, array);
    case xbus::node::conf::bind:
        return ::read_param<xbus::node::conf::bind_t>(stream, array);
    case xbus::node::conf::string:
        return ::read_param_str<xbus::node::conf::string_t>(stream, array);
    case xbus::node::conf::text:
        return ::read_param_str<xbus::node::conf::text_t>(stream, array);
    case xbus::node::conf::script:
        return ::read_param<xbus::node::conf::script_t>(stream, array);
    }
    return QVariant();
}

template<typename _T>
static bool write_param(ProtocolStreamWriter &stream, size_t array, const QVariant &value)
{
    if (array > 0) {
        const QVariantList &list = value.value<QVariantList>();
        if (static_cast<size_t>(list.size()) != array)
            return false;
        for (auto v : list) {
            if (!stream.write<_T>(v.value<_T>()))
                return false;
        }
        return true;
    }
    return stream.write<_T>(value.value<_T>());
}

template<typename _T>
static bool write_param_str(ProtocolStreamWriter &stream, size_t array, const QVariant &value)
{
    if (array > 0) {
        const QVariantList &list = value.value<QVariantList>();
        if (static_cast<size_t>(list.size()) != array)
            return false;
        for (auto v : list) {
            if (!stream.write_string(v.toString().toLatin1().data()))
                return false;
        }
        return true;
    }
    return stream.write_string(value.toString().toLatin1().data());
}

bool ProtocolNode::write_param(ProtocolStreamWriter &stream,
                               xbus::node::conf::fid_t fid,
                               const QVariant &value)
{
    const dict_field_s *f = field(fid);
    if (!f)
        return false;
    //qDebug() << f->name << stream.payload().toHex().toUpper();
    size_t array = f->array;
    switch (f->type) {
    case xbus::node::conf::group:
    case xbus::node::conf::command:
    case xbus::node::conf::type_max:
        break;
    case xbus::node::conf::option:
        return ::write_param<xbus::node::conf::option_t>(stream, array, value);
    case xbus::node::conf::real:
        return ::write_param<xbus::node::conf::real_t>(stream, array, value);
    case xbus::node::conf::byte:
        return ::write_param<xbus::node::conf::byte_t>(stream, array, value);
    case xbus::node::conf::word:
        return ::write_param<xbus::node::conf::word_t>(stream, array, value);
    case xbus::node::conf::dword:
        return ::write_param<xbus::node::conf::dword_t>(stream, array, value);
    case xbus::node::conf::bind:
        return ::write_param<xbus::node::conf::bind_t>(stream, array, value);
    case xbus::node::conf::string:
        return ::write_param_str<xbus::node::conf::string_t>(stream, array, value);
    case xbus::node::conf::text:
        return ::write_param_str<xbus::node::conf::text_t>(stream, array, value);
    case xbus::node::conf::script:
        return ::write_param<xbus::node::conf::script_t>(stream, array, value);
    }
    return false;
}

//---------------------------------------
// PROPERTIES
//---------------------------------------

const xbus::node::ident::ident_s &ProtocolNode::ident() const
{
    return m_ident;
}
bool ProtocolNode::setIdent(const xbus::node::ident::ident_s &ident)
{
    if (memcmp(&m_ident, &ident, sizeof(ident)) == 0)
        return false;
    m_ident = ident;
    setIdentValid(true);
    setDictValid(false);
    emit identChanged();
    return true;
}
QString ProtocolNode::sn() const
{
    return m_sn;
}
QString ProtocolNode::version() const
{
    return m_version;
}
void ProtocolNode::setVersion(const QString &v)
{
    if (m_version == v)
        return;
    m_version = v;
    emit versionChanged();
}
QString ProtocolNode::hardware() const
{
    return m_hardware;
}
void ProtocolNode::setHardware(const QString &v)
{
    if (m_hardware == v)
        return;
    m_hardware = v;
    emit hardwareChanged();
}
QStringList ProtocolNode::files() const
{
    return m_files;
}
void ProtocolNode::setFiles(const QStringList &v)
{
    if (m_files == v)
        return;
    m_files = v;
    emit filesChanged();

    if (m_files.isEmpty())
        return;

    emit filesAvailable();
    if (_nodes->active() && m_files.contains("fw")) {
        emit loaderAvailable();
    }
    for (auto i : m_files)
        file(i); // create fact
}
bool ProtocolNode::identValid() const
{
    return m_identValid;
}
void ProtocolNode::setIdentValid(const bool &v)
{
    if (m_identValid == v)
        return;
    m_identValid = v;
    if (!v) {
        setDictValid(false);
        setValid(false);
        resetFilesMap();
        setFiles(QStringList());
        memset(&m_ident, 0, sizeof(m_ident));
        emit identChanged();
    }
    emit identValidChanged();
}
bool ProtocolNode::dictValid() const
{
    return m_dictValid;
}
void ProtocolNode::setDictValid(const bool &v)
{
    if (m_dictValid == v)
        return;
    if (v && !identValid())
        return;
    if (!v) {
        m_dict.clear();
        m_dict_fields.clear();
        setValid(false);
    }
    m_dictValid = v;
    emit dictValidChanged();
}
bool ProtocolNode::valid() const
{
    return m_valid;
}
void ProtocolNode::setValid(const bool &v)
{
    if (m_valid == v)
        return;
    if (v && !dictValid())
        return;
    m_valid = v;
    emit validChanged();
}
