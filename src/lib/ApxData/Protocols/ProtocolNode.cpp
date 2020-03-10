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
    : ProtocolBase(nodes)
    , sn(sn)
    , nodes(nodes)
{
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
        if (stream.available()
            <= (xbus::node::ident::ident_s::psize() + xbus::node::ident::strings_count * 2)) {
            qWarning() << "size" << stream.available() << xbus::node::ident::ident_s::psize();
            break;
        }
        nodes->acknowledgeRequest(stream);
        m_ident.read(&stream);
        if (xbus::node::ident::strings_count != 3)
            break;
        // format ok
        const char *s;
        s = stream.read_string(32);
        if (!s)
            break;
        m_name = QString(s).trimmed();
        s = stream.read_string(32);
        if (!s)
            break;
        m_version = QString(s).trimmed();
        s = stream.read_string(32);
        if (!s)
            break;
        m_hardware = QString(s).trimmed();
        emit identReceived();
    } break;

        // request acknowledge
    case mandala::cmd::env::nmt::ack::uid: {
        if (stream.available() != sizeof(xbus::node::crc_t))
            break;
        nodes->acknowledgeRequest(stream.read<xbus::node::crc_t>());
    } break;

        // message from vehicle
    case mandala::cmd::env::nmt::msg::uid: {
        if (stream.available() < (sizeof(xbus::node::msg::type_t) + 1))
            break;
        xbus::node::msg::type_t t;
        stream >> t;
        QByteArray s_ba(static_cast<int>(stream.available()), '\0');
        stream.read(s_ba.data(), stream.available());

        QString s(s_ba.trimmed());
        if (s.isEmpty())
            break;
        emit messageReceived(t, s);
    } break;
    }
}

ProtocolNodeRequest *ProtocolNode::request(xbus::pid_t pid, int timeout_ms, int retry_cnt)
{
    return nodes->request(pid, sn, timeout_ms, retry_cnt);
}

const xbus::node::ident::ident_s &ProtocolNode::ident() const
{
    return m_ident;
}
QString ProtocolNode::name() const
{
    return m_name;
}
QString ProtocolNode::version() const
{
    return m_version;
}
QString ProtocolNode::hardware() const
{
    return m_hardware;
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
