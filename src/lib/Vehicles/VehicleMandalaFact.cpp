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
#include "Mandala.h"
#include "VehicleMandala.h"
//=============================================================================
VehicleMandalaFact::VehicleMandalaFact(VehicleMandala *parent, Mandala *m, quint16 id, DataType dataType, const QString &name, const QString &title, const QString &descr, const QString &units)
  : Fact(parent,name,title,descr,FactItem,dataType),
    vehicle(parent),m(m),m_id(id),
    m_units(units)
{
  connect(this,&VehicleMandalaFact::sendUplink,parent,&VehicleMandala::sendUplink);

  packed.reserve(sizeof(tmp));
  m->get_ptr(id,&_value_ptr,&_vtype);
  _unpackedValue=0;

  loadValueTimer.setSingleShot(true);
  loadValueTimer.setInterval(2000);
  connect(&loadValueTimer,&QTimer::timeout,this,&VehicleMandalaFact::loadValueDo);

  //limit send value bandwidth
  sendValueTimer.setSingleShot(true);
  sendValueTimer.setInterval(80);
  connect(&sendValueTimer,&QTimer::timeout,[=](){ emit sendUplink(packed); });
  connect(this,&VehicleMandalaFact::sendUplink,[=](){ sendValueTime.restart(); });
}
//=============================================================================
//=============================================================================
QString VehicleMandalaFact::units(void) const
{
  return m_units;
}
//=============================================================================
void VehicleMandalaFact::saveValue()
{
  m->set_data(m_id,_vtype,_value_ptr,value().toDouble());
}
//=============================================================================
void VehicleMandalaFact::loadValue() //value arrived from telemetry
{
  _unpackedValue=m->get_data(m_id,_vtype,_value_ptr);
  if(loadValueTimer.isActive())return;
  loadValueDo();
}
void VehicleMandalaFact::loadValueDo()
{
  setValueCnt=0;
  Fact::setValue(unpackedValue());
}
//=============================================================================
double VehicleMandalaFact::unpackedValue()
{
  return _unpackedValue;
}
//=============================================================================
//=============================================================================
bool VehicleMandalaFact::setValue(const QVariant &v)
{
  if(_bindedFact) return _bindedFact->setValue(v);
  if(!setValueLocal(v))return false;
  if(!pack())return false;
  if(setValueCnt++<5)loadValueTimer.start();
  //send uplink
  if(sendValueTimer.isActive())return true;
  if(sendValueTime.elapsed()>sendValueTimer.interval()){
    emit sendUplink(packed);
  }else sendValueTimer.start();
  return true;
}
//=============================================================================
bool VehicleMandalaFact::setValueLocal(const QVariant &v)
{
  return Fact::setValue(v);
}
//=============================================================================
bool VehicleMandalaFact::pack()
{
  saveValue();
  int sz=0;
  switch(_vtype){
    case vt_flag:
    case vt_float:
    case vt_vect:
    case vt_point:{
      tmp[0]=idx_set;
      sz=m->pack_set(tmp+1,m_id)+1;
    }break;
    default:
      tmp[0]=m_id;
      sz=m->pack(tmp+1,m_id)+1;
  }
  if(memcmp(tmp,packed.data(),sz)==0)return false;
  packed.clear();
  packed.append((const char*)tmp,sz);
  return true;
}
//=============================================================================
//=============================================================================
