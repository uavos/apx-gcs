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

#include <Fact/Fact.h>
#include <Mandala/MandalaMetaBase.h>

#include <xbus/XbusStreamReader.h>
#include <xbus/XbusStreamWriter.h>

class MandalaFactStreamBase
{
public:
    virtual ~MandalaFactStreamBase() {}
    virtual MandalaFactStreamBase &operator<<(XbusStreamReader &stream) = 0;
    virtual const MandalaFactStreamBase &operator>>(XbusStreamWriter &stream) const = 0;

    virtual void setValueFromStream(const QVariant &);
    virtual QVariant getValueForStream() const;
};

class MandalaFactStream : public MandalaFactStreamBase
{
public:
    explicit MandalaFactStream(const mandala::type_id_e &type_id);
    virtual ~MandalaFactStream() override;

    MandalaFactStreamBase *get_stream(const mandala::type_id_e &type_id);

    MandalaFactStream &operator<<(XbusStreamReader &stream) override;
    const MandalaFactStream &operator>>(XbusStreamWriter &stream) const override;

private:
    MandalaFactStreamBase *m_stream{nullptr};
};
