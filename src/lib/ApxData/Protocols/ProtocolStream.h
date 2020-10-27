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
#pragma once

#include <QtCore>

#include <xbus/XbusStreamReader.h>
#include <xbus/XbusStreamWriter.h>

#include <mandala/MandalaTreeBase.h>
#include <xbus/XbusPacket.h>

class ProtocolStreamReader : public XbusStreamReader
{
public:
    ProtocolStreamReader(const QByteArray &packet)
        : XbusStreamReader(reinterpret_cast<const uint8_t *>(packet.data()),
                           static_cast<size_t>(packet.size()))
    {}

    inline QString dump() { return toByteArray().toHex().toUpper(); }
    inline QString dump_payload() { return payload().toHex().toUpper(); }

    inline QByteArray toByteArray(size_t spos, size_t sz) const
    {
        if ((spos + sz) > size())
            return QByteArray();
        return QByteArray(reinterpret_cast<const char *>(buffer() + spos), static_cast<int>(sz));
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
        return QByteArray(reinterpret_cast<const char *>(buffer() + spos),
                          static_cast<int>(pos() - spos));
    }

    inline size_t append(const QByteArray &ba)
    {
        return XbusStreamWriter::write(ba.data(), static_cast<size_t>(ba.size()));
    }

    inline void req(mandala::uid_t uid, xbus::pri_e pri = xbus::pri_request)
    {
        reset();
        _req_pid.uid = uid;
        _req_pid.pri = pri;
        _req_pid.write(this);
        _req_pid.seq++;
    }

private:
    xbus::pid_s _req_pid{};
};
