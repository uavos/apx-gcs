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

#include "MandalaFactStream.h"

template<typename _DataType>
class MandalaFactStreamT : public MandalaFactStreamBase
{
public:
    explicit MandalaFactStreamT(MandalaFactStreamBase *base)
        : m_base(base)
    {}

protected:
    MandalaFactStreamT &operator<<(XbusStreamReader &stream) override
    {
        m_base->setValueFromStream(QVariant::fromValue(stream.read<_DataType>()));
        return *this;
    }
    const MandalaFactStreamT &operator>>(XbusStreamWriter &stream) const override
    {
        stream.write<_DataType>(m_base->getValueForStream().template value<_DataType>());
        return *this;
    }

private:
    MandalaFactStreamBase *m_base;
};

MandalaFactStream::MandalaFactStream(const mandala::type_id_e &type_id)
    : m_stream(get_stream(type_id))
{}
MandalaFactStream::~MandalaFactStream()
{
    if (m_stream)
        delete m_stream;
}

MandalaFactStreamBase *MandalaFactStream::get_stream(const mandala::type_id_e &type_id)
{
    switch (type_id) {
    default:
        return nullptr;
    case mandala::type_real:
        return new MandalaFactStreamT<mandala::real_t>(this);
    case mandala::type_dword:
        return new MandalaFactStreamT<mandala::dword_t>(this);
    case mandala::type_word:
        return new MandalaFactStreamT<mandala::word_t>(this);
    case mandala::type_byte:
        return new MandalaFactStreamT<mandala::byte_t>(this);
    case mandala::type_option:
        return new MandalaFactStreamT<mandala::option_t>(this);
    }
}

MandalaFactStream &MandalaFactStream::operator<<(XbusStreamReader &stream)
{
    *m_stream << stream;
    return *this;
}
const MandalaFactStream &MandalaFactStream::operator>>(XbusStreamWriter &stream) const
{
    *m_stream >> stream;
    return *this;
}

void MandalaFactStreamBase::setValueFromStream(const QVariant &) {}
QVariant MandalaFactStreamBase::getValueForStream() const
{
    return QVariant();
}
