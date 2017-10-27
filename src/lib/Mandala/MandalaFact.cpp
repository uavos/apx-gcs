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
#include "MandalaFact.h"
#include "Mandala.h"
#include "VehicleMandala.h"
//=============================================================================
MandalaFact::MandalaFact(VehicleMandala *parent, Mandala *m, quint16 id, DataType dataType, const QString &name, const QString &title, const QString &descr, const QString &units)
  : Fact(parent,name,title,descr,FactItem,dataType),
    vehicle(parent),m(m),m_id(id),
    m_units(units)
{
}
//=============================================================================
//=============================================================================
QString MandalaFact::units(void) const
{
  return m_units;
}
//=============================================================================
void MandalaFact::saveValue()
{
  m->set_data(m_id,value().toDouble());
}
//=============================================================================
void MandalaFact::loadValue()
{
  setValueLocal(m->get_data(m_id));
}
//=============================================================================
//=============================================================================
bool MandalaFact::setValue(const QVariant &v)
{

}
//=============================================================================
bool MandalaFact::setValueLocal(const QVariant &v)
{
  return Fact::setValue(v);
}
//=============================================================================
