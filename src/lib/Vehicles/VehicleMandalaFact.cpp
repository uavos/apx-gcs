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
#include "Vehicle.h"
//=============================================================================
VehicleMandalaFact::VehicleMandalaFact(VehicleMandala *parent, Mandala *m, quint16 id, DataType dataType, const QString &name, const QString &title, const QString &descr, const QString &units)
  : Fact(parent,name,title,descr,FactItem,dataType),
    vehicleMandala(parent),m(m),m_id(id)
{
  setUnits(units);

  connect(this,&VehicleMandalaFact::sendUplink,parent,&VehicleMandala::sendUplink);

  packed.reserve(sizeof(tmp));
  m->get_ptr(id,&_value_ptr,&_vtype);
  _unpackedValue=0;

  setPrecision(getPrecision());
  setColor(getColor());

  loadValueTimer.setSingleShot(true);
  loadValueTimer.setInterval(2000);
  connect(&loadValueTimer,&QTimer::timeout,this,&VehicleMandalaFact::loadValueDo);

  //limit send value bandwidth
  sendValueTimer.setSingleShot(true);
  sendValueTimer.setInterval(80);
  connect(&sendValueTimer,&QTimer::timeout,this,[=](){ emit sendUplink(packed); });
  connect(this,&VehicleMandalaFact::sendUplink,this,[=](){ sendValueTime.restart(); });

  QVariant v;
  switch(dataType){
    default: break;
    case FloatData: v=(double)0.0;break;
    case IntData: v=(int)0;break;
    case EnumData: v=(int)0;break;
    case BoolData: v=(bool)false;break;
  }
  setValueLocal(v);
}
//=============================================================================
uint VehicleMandalaFact::getPrecision()
{
  switch(_vtype){
    case vt_byte:
    case vt_uint:
    case vt_flag:
    case vt_enum:
    case vt_void:
    case vt_idx:
      return 0;
  }
  if(m_name.contains("lat")||m_name.contains("lon"))return 8;
  if(m_name=="ldratio") return 2;
  if(m_name.startsWith("ctr")) return 3;
  if(m_units=="0..1")   return 3;
  if(m_units=="-1..0..+1")return 3;
  if(m_units=="deg")    return 2;
  if(m_units=="deg/s")  return 2;
  if(m_units=="m")      return 2;
  if(m_units=="m/s")    return 2;
  if(m_units=="m/s2")   return 2;
  if(m_units=="a.u.")   return 2;
  if(m_units=="v")      return 2;
  if(m_units=="A")      return 3;
  if(m_units=="C")      return 1;
  return 6;
}
//=============================================================================
QColor VehicleMandalaFact::getColor()
{
  //fill params
  uint type=_vtype;
  uint varmsk=id();
  QString sn=name();
  uint ci=0;
  if(type==vt_vect || type==vt_point) ci=(varmsk>>8)+1;
  QColor c(Qt::cyan);
  if(sn.startsWith("ctr_")){
    if(sn.contains("ailerons"))c=QColor(Qt::red).lighter();
    else if(sn.contains("elevator"))c=QColor(Qt::green).lighter();
    else if(sn.contains("throttle"))c=QColor(Qt::blue).lighter();
    else if(sn.contains("rudder"))c=QColor(Qt::yellow).lighter();
    else if(sn.contains("collective"))c=QColor(Qt::darkCyan);
    else c=QColor(Qt::magenta).darker();
  }else if(sn.startsWith("rc_")){
    if(sn.contains("roll"))c=QColor(Qt::red);
    else if(sn.contains("pitch"))c=QColor(Qt::darkGreen);
    else if(sn.contains("throttle"))c=QColor(Qt::darkBlue);
    else if(sn.contains("yaw"))c=QColor(Qt::darkYellow);
    else c=QColor(Qt::magenta).lighter();
  }else if(sn.startsWith("ctrb_"))c=Qt::magenta;
  else if(sn.startsWith("user")){
    if(sn.endsWith("r1"))c=QColor(Qt::red).lighter();
    else if(sn.endsWith("r2"))c=QColor(Qt::green).lighter();
    else if(sn.endsWith("r3"))c=QColor(Qt::blue).lighter();
    else if(sn.endsWith("r4"))c=QColor(Qt::yellow).lighter();
    else if(sn.endsWith("r5"))c=QColor(Qt::cyan).lighter();
    else if(sn.endsWith("r6"))c=QColor(Qt::magenta).lighter();
    else c=QColor(Qt::cyan).lighter();
  }else if(sn.startsWith("altitude"))c=Qt::red;
  else if(sn.startsWith("vspeed"))c=Qt::green;
  else if(sn.startsWith("airspeed"))c=Qt::blue;
  else if(type==vt_flag)c=QColor(Qt::blue).lighter();
  else if(ci==1)c=Qt::red;
  else if(ci==2)c=Qt::green;
  else if(ci==3)c=Qt::yellow;
  return c;
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
  if(vehicleMandala->vehicle->isReplay()) return true;
  if(!pack())return false;
  if(setValueCnt++<5)loadValueTimer.start();
  //qDebug()<<"set"<<path();
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
