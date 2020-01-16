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
//=============================================================================
#include <Mandala/MandalaMetaBase.h>
#include <Mandala/MandalaStream.h>
#include <QtCore>

class MandalaTreeStream
{
public:
    explicit MandalaTreeStream(QString _type_text, QString _sfmt_text)
        : type_text(_type_text)
        , sfmt_text(_sfmt_text)
    {}
    virtual size_t pack(void *buf, const QVariant &v) const = 0;
    virtual size_t unpack(const void *buf, QVariant &v) = 0;
    virtual size_t psize() const = 0;
    QString type_text;
    QString sfmt_text;
};

template<mandala::sfmt_id_t _sfmt, typename _DataType>
class MandalaTreeStreamT : public MandalaTreeStream
{
public:
    explicit MandalaTreeStreamT(QString _type_text, QString _sfmt_text)
        : MandalaTreeStream(_type_text, _sfmt_text)
    {}

protected:
    size_t pack(void *buf, const QVariant &v) const override
    {
        return mandala::stream::pack<_sfmt, _DataType>(buf, v.value<_DataType>());
    }
    size_t unpack(const void *buf, QVariant &v) override
    {
        _DataType d;
        size_t sz = mandala::stream::unpack<_sfmt, _DataType>(buf, d);
        if (sz > 0)
            v = QVariant::fromValue(d);
        return sz;
    }
    size_t psize() const override { return mandala::stream::psize<_sfmt>(); }
};

template<mandala::sfmt_id_t _sfmt>
MandalaTreeStream *get_stream(mandala::type_id_t type, QString sfmt_text)
{
    switch (type) {
    case mandala::type_float:
        return new MandalaTreeStreamT<_sfmt, mandala::float_t>("float", sfmt_text);
    case mandala::type_dword:
        return new MandalaTreeStreamT<_sfmt, mandala::dword_t>("dword", sfmt_text);
    case mandala::type_word:
        return new MandalaTreeStreamT<_sfmt, mandala::word_t>("word", sfmt_text);
    case mandala::type_byte:
        return new MandalaTreeStreamT<mandala::sfmt_u1, mandala::byte_t>("byte", sfmt_text);
    case mandala::type_enum:
        return new MandalaTreeStreamT<mandala::sfmt_u1, mandala::enum_t>("enum", sfmt_text);
    }
}
