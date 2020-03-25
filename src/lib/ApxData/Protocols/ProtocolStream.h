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
#pragma once

#include <QtCore>

#include <Xbus/XbusStreamReader.h>
#include <Xbus/XbusStreamWriter.h>

#include <Mandala/MandalaTreeBase.h>
#include <Xbus/XbusPacket.h>

class ProtocolStreamReader : public XbusStreamReader
{
public:
    ProtocolStreamReader(const QByteArray &packet)
        : XbusStreamReader(reinterpret_cast<const uint8_t *>(packet.data()),
                           static_cast<size_t>(packet.size()))
        , m_packet(packet)
    {}

    inline QString dump() { return toByteArray().toHex().toUpper(); }

    inline QByteArray toByteArray(size_t spos, size_t sz) const
    {
        if ((spos + sz) > size())
            return QByteArray();
        return m_packet.mid(static_cast<int>(spos), static_cast<int>(sz));
        //QByteArray::fromRawData(reinterpret_cast<const char *>(buffer() + spos),
        //                        static_cast<int>(sz));
    }

    inline QByteArray toByteArray() const { return toByteArray(0, pos()); }

    inline QByteArray payload() const { return toByteArray(pos(), available()); }

    QStringList read_strings(size_t cnt, size_t max_sz = 64)
    {
        QStringList st;
        for (size_t i = 0; cnt == 0 || i < cnt; ++i) {
            const char *s;
            s = read_string(max_sz);
            if (!s)
                break;
            st.append(QString(s).trimmed());
        }
        if (cnt > 0 && st.size() != static_cast<int>(cnt))
            st.clear();
        return st;
    }

private:
    const QByteArray &m_packet;
};

class ProtocolStreamWriter : public XbusStreamWriter
{
public:
    explicit ProtocolStreamWriter(void *p, size_t size)
        : XbusStreamWriter(p, size)
    {}

    inline QString dump(size_t spos = 0) { return toByteArray(spos).toHex().toUpper(); }

    inline QByteArray toByteArray(size_t spos = 0) const
    {
        if (spos >= size())
            return QByteArray();
        return QByteArray::fromRawData(reinterpret_cast<const char *>(buffer() + spos),
                                       static_cast<int>(pos() - spos));
    }

    inline size_t append(const QByteArray &ba)
    {
        return XbusStreamWriter::write(ba.data(), static_cast<size_t>(ba.size()));
    }

    inline void req(mandala::uid_t uid, xbus::sub_e sub = xbus::sub_request)
    {
        reset();
        _req_pid.uid = uid;
        _req_pid.sub = sub;
        _req_pid.write(this);
        _req_pid.seq++;
    }

private:
    xbus::pid_s _req_pid{};
};
