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

    requestIdent();
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
        // format ok
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

        setName(sname.toLower());
        setTitle(sname);

        if (m_version != sversion) {
            m_version = sversion;
            emit versionChanged();
        }
        if (m_hardware != shardware) {
            m_hardware = shardware;
            emit hardwareChanged();
        }

        fnames.sort();
        if (memcmp(&m_ident, &ident, sizeof(ident)) != 0 || fnames != m_files.keys()) {
            m_identValid = true;
            m_ident = ident;
            updateFiles(fnames);
            emit identChanged();
        }
        emit identReceived();
        nodes->nodeUpdate(this);
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

void ProtocolNode::updateFiles(QStringList fnames)
{
    qDebug() << "files:" << m_ident.flags.bits.files << fnames;
    qDeleteAll(m_files.values());
    m_files.clear();
    for (auto i : fnames) {
        ProtocolNodeFile *file = new ProtocolNodeFile(this, i);
        m_files.insert(i, file);
        connect(nodes, &ProtocolNodes::stopRequested, file, &ProtocolNodeFile::stop);
    }
    if (nodes->active() && file("fw")) {
        emit loaderAvailable();
    }
}

ProtocolNodeRequest *ProtocolNode::request(xbus::pid_t pid, int timeout_ms, int retry_cnt)
{
    return nodes->request(pid, m_sn, timeout_ms, retry_cnt);
}

QString ProtocolNode::sn() const
{
    return m_sn;
}
const xbus::node::ident::ident_s &ProtocolNode::ident() const
{
    return m_ident;
}
bool ProtocolNode::identValid() const
{
    return m_identValid;
}
QString ProtocolNode::version() const
{
    return m_version;
}
QString ProtocolNode::hardware() const
{
    return m_hardware;
}
ProtocolNodeFile *ProtocolNode::file(const QString &name) const
{
    return m_files.value(name, nullptr);
}

void ProtocolNode::requestReboot()
{
    nodes->stop();
    nodes->setActive(true);
    ProtocolNodeRequest *req = request(mandala::cmd::env::nmt::reboot::uid);
    req->write<xbus::node::reboot::type_e>(xbus::node::reboot::firmware);
    req->schedule();
}
void ProtocolNode::requestRebootLoader()
{
    resetIdent();
    nodes->stop();
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
void ProtocolNode::resetIdent()
{
    memset(&m_ident, 0, sizeof(m_ident));
    m_identValid = false;
    updateFiles(QStringList());
    emit identChanged();
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
