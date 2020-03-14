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
#include <Xbus/XbusNodeConf.h>

#include <crc/crc.h>

ProtocolNode::ProtocolNode(ProtocolNodes *nodes, const QString &sn)
    : ProtocolBase(nodes, "node")
    , nodes(nodes)
    , m_sn(sn)
{
    setIcon("sitemap");

    memset(&m_ident, 0, sizeof(m_ident));

    connect(nodes, &ProtocolNodes::activeChanged, this, [this]() {
        if (!this->nodes->active())
            setProgress(-1);
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

void ProtocolNode::downlink(xbus::pid_t pid, ProtocolStreamReader &stream)
{
    //filter requests
    if (stream.available() == 0 && pid != mandala::cmd::env::nmt::search::uid)
        return;

    //qDebug() << QString("[%1]").arg(Mandala::meta(pid).name) << stream.available();

    switch (pid) {
    default:
        //qDebug() << cmd << data.size();
        return;

    case mandala::cmd::env::nmt::search::uid: { //response to search
        //qDebug() << "apc_search" << sn;
        requestIdent();
    } break;

        // node ident
    case mandala::cmd::env::nmt::ident::uid: {
        //qDebug() << "ident" << sn;
        if (stream.available() <= (xbus::node::ident::ident_s::psize() + 3 * 2)) {
            qWarning() << "size" << stream.available() << xbus::node::ident::ident_s::psize();
            break;
        }
        nodes->acknowledgeRequest(stream);
        xbus::node::ident::ident_s ident;
        ident.read(&stream);

        const char *s;
        s = stream.read_string(32);
        if (!s)
            break;
        QString sname = QString(s).trimmed();

        s = stream.read_string(32);
        if (!s)
            break;
        QString sversion = QString(s).trimmed();

        s = stream.read_string(32);
        if (!s)
            break;
        QString shardware = QString(s).trimmed();

        QStringList fnames;
        for (size_t cnt = ident.flags.bits.files; cnt > 0; --cnt) {
            s = stream.read_string(32);
            if (!s)
                break;
            QString sf = QString(s).trimmed();
            if (sf.isEmpty())
                break;
            fnames.append(sf);
        }
        if (stream.available() > 0)
            break;
        if (fnames.size() != ident.flags.bits.files)
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
        if (stream.available() != sizeof(xbus::node::crc_t))
            break;
        nodes->acknowledgeRequest(stream.read<xbus::node::crc_t>());
    } break;
    case mandala::cmd::env::nmt::nack::uid: {
        if (stream.available() <= sizeof(xbus::node::crc_t))
            break;
        xbus::node::crc_t crc;
        stream >> crc;
        if (stream.available() != sizeof(uint16_t))
            break;
        uint16_t ms;
        stream >> ms;
        nodes->extendRequest(crc, ms);
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

        const char *s = stream.read_string(16);
        if (!s)
            break;
        ProtocolNodeFile *f = file(s);
        if (!f)
            break;
        f->downlink(op, stream);
    } break;

        // message from vehicle
    case mandala::cmd::env::nmt::msg::uid: {
        if (stream.available() < (sizeof(xbus::node::msg::type_t) + 1))
            break;
        xbus::node::msg::type_t t;
        stream >> t;
        const char *s = stream.read_string(stream.available());
        QString msg(QString(s).trimmed());
        if (msg.isEmpty())
            break;
        emit messageReceived(t, msg);
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
    connect(nodes, &ProtocolNodes::stopRequested, f, &ProtocolNodeFile::stop);
    return f;
}

ProtocolNodeRequest *ProtocolNode::request(xbus::pid_t pid, int timeout_ms, int retry_cnt)
{
    return nodes->request(pid, m_sn, timeout_ms, retry_cnt);
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
    timeReqLoader.start();
    requestRebootLoaderNext();
}
void ProtocolNode::requestRebootLoaderNext()
{
    qDebug() << "ldr";
    if (timeReqLoader.elapsed() > 10000) {
        nodes->setActive(false);
        return;
    }
    nodes->clear_requests();
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::reboot::uid, 0);
    req->write<xbus::node::reboot::type_e>(xbus::node::reboot::loader);
    connect(req, &ProtocolNodeRequest::finished, this, [this]() {
        ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::ident::uid, 0);
        connect(req, &ProtocolNodeRequest::finished, this, [this]() {
            if (!file("fw") && nodes->active())
                requestRebootLoaderNext();
        });
        req->schedule();
    });
    req->schedule();
}

void ProtocolNode::requestIdent()
{
    request(mandala::cmd::env::nmt::ident::uid)->schedule();
}
void ProtocolNode::requestDict()
{
    qDebug() << "file download";
}
void ProtocolNode::requestConf()
{
    qDebug() << "file download";
}
void ProtocolNode::requestStatus()
{
    request(mandala::cmd::env::nmt::status::uid)->schedule();
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

    if (!m_files.isEmpty())
        emit filesAvailable();
    if (nodes->active() && m_files.contains("fw")) {
        emit loaderAvailable();
    }
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
        setDataValid(false);
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
        setDataValid(false);
    }
    m_dictValid = v;
    emit dictValidChanged();
}
bool ProtocolNode::dataValid() const
{
    return m_dataValid;
}
void ProtocolNode::setDataValid(const bool &v)
{
    if (m_dataValid == v)
        return;
    if (v && !dictValid())
        return;
    m_dataValid = v;
    emit dataValidChanged();
}
bool ProtocolNode::upgrading() const
{
    return m_upgrading;
}
void ProtocolNode::setUpgrading(const bool &v)
{
    if (m_upgrading == v)
        return;
    m_upgrading = v;
    emit upgradingChanged();
}
