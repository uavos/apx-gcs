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

class PStreamReader : public XbusStreamReader
{
public:
    PStreamReader(const QByteArray &packet)
        : XbusStreamReader(reinterpret_cast<const uint8_t *>(packet.data()),
                           static_cast<size_t>(packet.size()))
    {}

    inline QString dump_header() { return header().toHex().toUpper(); }
    inline QString dump_payload() { return payload().toHex().toUpper(); }
    inline QString dump_all() { return all().toHex().toUpper(); }

    inline QByteArray toByteArray(size_t spos, size_t sz) const
    {
        if ((spos + sz) > size())
            return QByteArray();
        return QByteArray(reinterpret_cast<const char *>(buffer() + spos), static_cast<int>(sz));
    }

    inline QByteArray header() const { return toByteArray(0, pos()); }
    inline QByteArray payload() const { return toByteArray(pos(), available()); }
    inline QByteArray all() const { return toByteArray(0, available()); }

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

class PStreamWriter : public XbusStreamWriter
{
public:
    explicit PStreamWriter(void *p, size_t size)
        : XbusStreamWriter(p, size)
    {}

    inline QString dump(size_t from = 0) { return toByteArray(from).toHex().toUpper(); }

    inline QByteArray toByteArray(size_t from = 0) const
    {
        if (from >= size())
            return QByteArray();
        return QByteArray(reinterpret_cast<const char *>(buffer() + from),
                          static_cast<int>(pos() - from));
    }

    inline size_t append(const QByteArray &ba)
    {
        return XbusStreamWriter::write(ba.data(), static_cast<size_t>(ba.size()));
    }
};
