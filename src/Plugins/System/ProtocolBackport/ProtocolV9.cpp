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
#include "ProtocolV9.h"

#include <xbus/XbusPacket.h>
#include <xbus/XbusVehicle.h>

#include <Mandala/backport/MandalaBackport.h>

namespace xbus {
namespace v9 {

typedef uint8_t pid_t;

}; // namespace v9
}; // namespace xbus

ProtocolV9::ProtocolV9(QObject *parent)
    : ProtocolConverter(parent)
    , out(xbus::size_packet_max, '\0')
    , stream(reinterpret_cast<uint8_t *>(out.data()))
{
    //id maps
    quint16 id = mandala::backport::uid_base;
    for (auto const &i : mandala::backport::items) {
        quint16 id_v9 = static_cast<quint16>(i.id_msb << 8) | id;
        downlinkIdMap.insert(id_v9, i.meta.uid);
        uplinkIdMap.insert(i.meta.uid, id_v9);
        if (i.id_msb == 0)
            id++;
        //qDebug() << id_v9 << i.meta.path;
    }
}

void ProtocolV9::convertDownlink(const QByteArray &packet)
{
    uint16_t psize = static_cast<uint16_t>(packet.size());
    const uint8_t *pdata = reinterpret_cast<const uint8_t *>(packet.data());
    XbusStreamReader is(pdata, psize);

    stream.reset();
    parseDownlink(is);
    if (!stream.position())
        return;
    const QByteArray &ba = out.left(stream.position());
    //qDebug() << "rx" << ba.toHex().toUpper();
    emit downlink(ba);
}

void ProtocolV9::convertUplink(const QByteArray &packet)
{
    uint16_t psize = static_cast<uint16_t>(packet.size());
    const uint8_t *pdata = reinterpret_cast<const uint8_t *>(packet.data());
    XbusStreamReader is(pdata, psize);

    stream.reset();
    parseUplink(is);
    if (!stream.position())
        return;
    const QByteArray &ba = out.left(stream.position());
    //qDebug() << "tx" << ba.toHex().toUpper();
    emit uplink(ba);
}

void ProtocolV9::copy(const XbusStreamReader &is)
{
    uint16_t sz = is.tail();
    if (!sz)
        return;
    memcpy(stream.data(), is.data(), sz);
    stream.reset(stream.position() + sz);
}

bool ProtocolV9::convertDownlinkId(quint16 *id)
{
    if (!downlinkIdMap.contains(*id))
        return false;
    *id = downlinkIdMap.value(*id);
    return true;
}
bool ProtocolV9::convertUplinkId(quint16 *id)
{
    if (!uplinkIdMap.contains(*id))
        return false;
    *id = uplinkIdMap.value(*id);
    return true;
}

void ProtocolV9::parseDownlink(XbusStreamReader &is)
{
    if (is.tail() < sizeof(xbus::v9::pid_t)) {
        return;
    }
    xbus::v9::pid_t pid = is.read<xbus::v9::pid_t>();

    switch (pid) {
    default:
        qWarning() << "pid" << pid << is.position();
        return;
    case mandala::backport::idx_service:
        stream.write<xbus::pid_t>(mandala::cmd::env::nmt::meta.uid);
        copy(is);
        break;
    case mandala::backport::idx_xpdr:
        stream.write<xbus::pid_t>(mandala::cmd::env::vehicle::xpdr::meta.uid);
        copy(is);
        break;
    case mandala::backport::idx_ident:
        stream.write<xbus::pid_t>(mandala::cmd::env::vehicle::ident::meta.uid);
        copy(is);
        break;
    case mandala::backport::idx_dlink:
        stream.write<xbus::pid_t>(mandala::cmd::env::vehicle::downlink::meta.uid);
        stream << is.read<xbus::vehicle::squawk_t>();
        parseDownlink(is);
        break;
    }
}

void ProtocolV9::parseUplink(XbusStreamReader &is)
{
    if (is.tail() < sizeof(xbus::pid_t)) {
        return;
    }
    xbus::pid_t pid = is.read<xbus::pid_t>();

    switch (pid) {
    default:
        qWarning() << "pid" << pid;
        return;
    case mandala::cmd::env::nmt::meta.uid:
        stream.write<xbus::v9::pid_t>(mandala::backport::idx_service);
        copy(is);
        break;
    case mandala::cmd::env::vehicle::xpdr::meta.uid:
        stream.write<xbus::v9::pid_t>(mandala::backport::idx_xpdr);
        copy(is);
        break;
    case mandala::cmd::env::vehicle::ident::meta.uid:
        stream.write<xbus::v9::pid_t>(mandala::backport::idx_ident);
        copy(is);
        break;
    case mandala::cmd::env::vehicle::uplink::meta.uid:
        stream.write<xbus::v9::pid_t>(mandala::backport::idx_dlink);
        stream << is.read<xbus::vehicle::squawk_t>();
        parseUplink(is);
        break;
    }
}
