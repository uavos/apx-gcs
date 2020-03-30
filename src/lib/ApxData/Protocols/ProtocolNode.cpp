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

ProtocolNode::ProtocolNode(ProtocolNodes *nodes, const QString &sn)
    : ProtocolBase(nodes, "node")
    , nodes(nodes)
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

    requestIdent();
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
    return ProtocolBase::toolTip().append("\n").append(st.join('\n'));
}
void ProtocolNode::hashData(QCryptographicHash *h) const
{
    ProtocolBase::hashData(h);
    h->addData(version().toUtf8());
    h->addData(hardware().toUtf8());
    h->addData(QString::number(ident().hash).toUtf8());
}

void ProtocolNode::downlink(const xbus::pid_s &pid, ProtocolStreamReader &stream)
{
    //filter requests
    while (stream.available() == 0) {
        if (pid.uid == mandala::cmd::env::nmt::search::uid)
            break;
        return;
    }

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
        nodes->acknowledgeRequest(m_sn, pid);
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
        nodes->nodeNotify(this);
    } break;

        // request acknowledge
    case mandala::cmd::env::nmt::ack::uid: {
        trace_downlink(stream.payload());
        if (stream.available() <= sizeof(xbus::pid_s))
            break;
        xbus::pid_s ack_pid;
        ack_pid.read(&stream);
        if (stream.available() < sizeof(xbus::node::ack::ack_e))
            break;
        xbus::node::ack::ack_e ack;
        stream >> ack;
        xbus::node::ack::timeout_t timeout = 0;
        if (ack == xbus::node::ack::ack_extend) {
            if (stream.available() != sizeof(xbus::node::ack::timeout_t))
                break;
            stream >> timeout;
        }
        nodes->acknowledgeRequest(m_sn, ack_pid, ack, timeout);
    } break;

        // file operations
    case mandala::cmd::env::nmt::file::uid: {
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
        emit messageReceived(t, msg);
    } break;

        // status from node
    case mandala::cmd::env::nmt::status::uid: {
        if (stream.available() != sizeof(xbus::node::status::status_s))
            break;
        nodes->acknowledgeRequest(m_sn, pid);

        trace_downlink(stream.payload());

        xbus::node::status::status_s status;
        status.read(&stream);

        emit statusReceived(status);
    } break;
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
    connect(nodes, &Fact::activeChanged, f, [f]() { f->stop(); });
    return f;
}

ProtocolNodeRequest *ProtocolNode::request(mandala::uid_t uid, size_t retry_cnt)
{
    return nodes->request(uid, m_sn, retry_cnt);
}

void ProtocolNode::requestReboot()
{
    nodes->clear_requests();
    nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::reboot::uid);
    req->write<xbus::node::reboot::type_e>(xbus::node::reboot::firmware);
    req->schedule();
}
void ProtocolNode::requestRebootLoader()
{
    setIdentValid(false);
    nodes->clear_requests();
    nodes->setActive(true);
    setProgress(0);
    timeReqLoader.start();
    requestRebootLoaderNext();
}
void ProtocolNode::requestRebootLoaderNext()
{
    qDebug() << "ldr";
    if (timeReqLoader.elapsed() > 10000) {
        qWarning() << "timeout";
        nodes->setActive(false);
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
                    if (!file("fw") && nodes->active())
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
    request(mandala::cmd::env::nmt::ident::uid)->schedule();
}
void ProtocolNode::requestDict()
{
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
void ProtocolNode::requestStatus(xbus::node::status::type_e type)
{
    nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::status::uid);
    *req << type;
    req->schedule();
}

void ProtocolNode::requestUpdate(xbus::node::conf::fid_t fid, QVariant value)
{
    const dict_field_s *f = field(fid);
    if (!f)
        return;
    //qDebug() << f->name << value;
    nodes->clear_requests();
    nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::upd::uid);
    *req << fid;
    if (!write_param(*req, fid, value)) {
        req->deleteLater();
        nodes->clear_requests();
        return;
    }
    connect(req, &ProtocolNodeRequest::timeout, nodes, &ProtocolNodes::clear_requests);
    req->schedule();
}
void ProtocolNode::requestUpdateSave()
{
    if (!nodes->active())
        return;
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::upd::uid);
    req->write<xbus::node::conf::fid_t>(0xFFFF);
    connect(req, &ProtocolNodeRequest::finished, this, [this](ProtocolNodeRequest *request) {
        if (request->acknowledged)
            emit confSaved();
        else
            nodes->clear_requests();
    });
    req->schedule();
}

void ProtocolNode::requestUsr(xbus::node::usr::cmd_t cmd, QByteArray data)
{
    nodes->setActive(true);
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
    do {
        // check node hash
        if (info.hash != m_ident.hash) {
            qWarning() << "node hash error:" << QString::number(info.hash, 16)
                       << QString::number(m_ident.hash, 16);
            break;
        }

        while (stream.available() > 4) {
            dict_field_s field;

            uint8_t v;
            stream >> v;
            field.type = v & 0x0F;
            field.array = v >> 4;
            stream >> v;
            field.group = v;

            QStringList st;

            switch (field.type) {
            case xbus::node::conf::group:
                st = stream.read_strings(3);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "group" << field.group << st;
                field.name = st.at(0);
                field.title = st.at(1);
                field.descr = st.at(2);
                groups.append(m_dict.size());
                break;
            case xbus::node::conf::command:
                st = stream.read_strings(2);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                field.name = st.at(0);
                field.title = st.at(1);
                break;
            default:
                st = stream.read_strings(3);
                if (st.isEmpty() || st.at(0).isEmpty())
                    break;
                //qDebug() << "field" << field.type << field.array << field.group << st;
                field.name = st.at(0);
                field.title = st.at(0);
                field.descr = st.at(1);
                field.units = st.at(2);
                m_dict_fields.append(m_dict.size());
                // guess name prepended with groups
                uint8_t group = field.group;
                while (group) {
                    group--;
                    if (group < groups.size()) {
                        const dict_field_s &g = m_dict.at(groups.at(group));
                        field.name.prepend(QString("%1_").arg(g.name));
                        group = g.group;
                        continue;
                    }
                    qWarning() << "missing group:" << field.type << field.array << field.group
                               << st;
                    field.name.clear(); //mark error
                    break;
                }
            }
            if (field.name.isEmpty())
                break;

            m_dict.append(field);

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
    setDictValid(true);
    emit dictReceived(m_dict);

    requestConf();
}

void ProtocolNode::parseConfData(const xbus::node::file::info_s &info, const QByteArray data)
{
    ProtocolStreamReader stream(data);
    bool err = true;
    QVariantList values;
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

        xbus::node::conf::fid_t fid = 0;
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

            values.append(v);
            fid++;

            if (fid == m_dict_fields.size()) {
                err = false;
                break;
            }
        }

    } while (0);

    if (err) {
        setValid(false);
        qWarning() << "conf error" << data.toHex().toUpper();
        return;
    }
    qDebug() << "conf parsed";
    setValid(true);
    emit confReceived(values);
}

const ProtocolNode::dict_field_s *ProtocolNode::field(xbus::node::conf::fid_t fid) const
{
    if (fid >= m_dict_fields.size())
        return nullptr;
    return &m_dict.at(m_dict_fields.at(fid));
}

template<typename _T>
static QVariant read_param(ProtocolStreamReader &stream, size_t array)
{
    if (stream.available() < (sizeof(_T) * (array ? array : 1)))
        return QVariant();
    if (array > 0) {
        QVariantList list;
        while (array--) {
            list.append(QVariant::fromValue(stream.read<_T>()));
        }
        return QVariant::fromValue(list);
    }
    return QVariant::fromValue(stream.read<_T>());
}
template<typename _T>
static QVariant read_param_str(ProtocolStreamReader &stream, size_t array)
{
    if (stream.available() < (sizeof(_T) * (array ? array : 1)))
        return QVariant();
    if (array > 0) {
        QVariantList list;
        while (array--) {
            const char *s = stream.read_string(sizeof(_T));
            if (!s)
                return QVariant();
            list.append(QVariant::fromValue(QString(s)));
        }
        return QVariant::fromValue(list);
    }
    const char *s = stream.read_string(sizeof(_T));
    return s ? QVariant::fromValue(QString(s)) : QVariant();
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
        break;
    case xbus::node::conf::option:
        return ::read_param<xbus::node::conf::option_t>(stream, array);
    case xbus::node::conf::real:
        return ::read_param<xbus::node::conf::real_t>(stream, array);
    case xbus::node::conf::byte:
        return ::read_param<xbus::node::conf::byte_t>(stream, array);
    case xbus::node::conf::word:
        return ::read_param<xbus::node::conf::word_t>(stream, array);
    case xbus::node::conf::dword:
        return ::read_param<xbus::node::conf::dword_t>(stream, array);
    case xbus::node::conf::mandala:
        return ::read_param<xbus::node::conf::mandala_t>(stream, array);
    case xbus::node::conf::string:
        return ::read_param_str<xbus::node::conf::string_t>(stream, array);
    case xbus::node::conf::text:
        return ::read_param_str<xbus::node::conf::text_t>(stream, array);
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
    case xbus::node::conf::mandala:
        return ::write_param<xbus::node::conf::mandala_t>(stream, array, value);
    case xbus::node::conf::string:
        return ::write_param_str<xbus::node::conf::string_t>(stream, array, value);
    case xbus::node::conf::text:
        return ::write_param_str<xbus::node::conf::text_t>(stream, array, value);
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
    if (nodes->active() && m_files.contains("fw")) {
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
