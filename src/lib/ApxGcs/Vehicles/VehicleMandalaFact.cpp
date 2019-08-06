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
#include "VehicleMandalaFact.h"
#include "Vehicle.h"
#include "VehicleMandala.h"
//=============================================================================
VehicleMandalaFact::VehicleMandalaFact(VehicleMandala *parent,
                                       quint16 id,
                                       const QString &name,
                                       const QString &title,
                                       const QString &descr,
                                       const QString &units,
                                       Flags flags)
    : Fact(parent, name, title, descr, flags)
    , vehicleMandala(parent)
    , m_id(id)
{
    setUnits(units);
    QVariant v = static_cast<double>(0.0);
    switch (dataType()) {
    default:
        break;
    case Int:
        v = static_cast<int>(0);
        break;
    case Enum:
        v = static_cast<int>(0);
        break;
    case Bool:
        v = static_cast<bool>(false);
        break;
    }
    setValueLocal(v);
}
//=============================================================================
bool VehicleMandalaFact::setValue(const QVariant &v)
{
    //always send uplink
    bool rv = Fact::setValue(v);
    send();
    //qDebug()<<name()<<text();
    return rv;
}
//=============================================================================
bool VehicleMandalaFact::setValueLocal(const QVariant &v)
{
    //don't send uplink
    return Fact::setValue(v);
}
//=============================================================================
quint16 VehicleMandalaFact::id()
{
    return m_id;
}
void VehicleMandalaFact::request()
{
    emit sendValueRequest(m_id);
}
void VehicleMandalaFact::send()
{
    emit sendValueUpdate(m_id, value().toDouble());
}
//=============================================================================
