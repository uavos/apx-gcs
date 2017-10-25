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
#include "QMandalaField.h"
#include "QMandalaItem.h"
#include "QMandala.h"
#include "Datalink.h"
#include <math.h>
//=============================================================================
char QMandalaField::skipWriteFields[]={idx_stage,0};
QMandalaField::QMandalaField()
  :QObject(),m_varmsk(0),m_value(0),m_name(""),m_type(0),m_precision(0),value_ptr(0)
{ //void field
}
QMandalaField::QMandalaField(QMandalaItem *parent, uint var_msk, QString name, QString descr, QString units)
  :QObject(),mvar(parent),m_varmsk(var_msk),m_value(0),m_name(name),m_descr(descr),m_units(units)
{
  //TODO: this is temporary?
  isSkipWriteField=strchr(skipWriteFields,m_varmsk&0xFF);

  updateMandalaPtr(mvar);
  setObjectName(m_name);
  updateMandalaPtr(parent);
  connect(mvar,SIGNAL(updated(uint)),SLOT(update()));

  setValueUpdateTimer.setSingleShot(true);
  setValueUpdateTimer.setInterval(2000);
  connect(&setValueUpdateTimer,SIGNAL(timeout()),this,SLOT(update()));

  setValueDelayTimer.setSingleShot(true);
  setValueDelayTimer.setInterval(50);
  connect(&setValueDelayTimer,SIGNAL(timeout()),this,SLOT(setValueDelayTimeout()));
  setValueTime.start();
}
QMandalaField::QMandalaField(QMandalaItem *parent, uint var_msk)
  :QObject(),mvar(parent),m_varmsk(var_msk),m_value(0),m_precision(0)
{
  if(m_varmsk<idxPAD || (!mvar->get_ptr(m_varmsk,&value_ptr,&m_type)))
    value_ptr=NULL;
  setObjectName(name());
  connect(mvar,SIGNAL(updated(uint)),SLOT(update()));
}
void QMandalaField::updateMandalaPtr(QMandalaItem *m)
{
  if(m_varmsk<idxPAD || (!m->get_ptr(m_varmsk,&value_ptr,&m_type))){
    value_ptr=NULL;
    m_type=0;
  }
  m_precision=getPrecision();
  m_text=(m_type==vt_enum)?m->enumName(m_varmsk,m_value):QString::number(m_value);
  m_caption=m_descr;
  if(m_caption.contains('['))
    m_caption=m_caption.remove(m_caption.indexOf('['),m_caption.size()).trimmed();
  //qDebug()<<m_name<<m_units<<m_precision;
}
uint QMandalaField::getPrecision()
{
  switch(m_type){
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
double QMandalaField::value()
{
  return m_value;
}
void QMandalaField::update()
{
  if(!value_ptr)return;
  if(setValueUpdateTimer.isActive()){
    if(++updateCnt>5)setValueUpdateTimer.stop();
    else return;
  }
  updateValue(mvar->get_data(m_varmsk,m_type,value_ptr));
}
void QMandalaField::updateValue(double v)
{
  if(v==m_value)return;
  if(!matrixmath::f_isvalid(v))v=0;
  m_value=v;
  m_text=(m_type==vt_enum)?mvar->enumName(m_varmsk,m_value):QString::number(m_value);
  //if(treeField)treeField->setValue(v);
  emit changed();
}
QStringList QMandalaField::enumStrings() const
{
  if(m_type!=vt_enum) return QStringList();
  return mvar->enumnames.value(m_varmsk);
}
uint QMandalaField::type()
{
  return m_type;
}
uint QMandalaField::precision()
{
  return m_precision;
}
uint QMandalaField::varmsk()
{
  return m_varmsk;
}
QString QMandalaField::name()
{
  return m_name;
}
QString QMandalaField::descr()
{
  return m_descr;
}
QString QMandalaField::units()
{
  return m_units;
}
QString QMandalaField::caption()
{
  return m_caption;
}
QString QMandalaField::text()
{
  return m_text;
}
void QMandalaField::setValue(double v)
{
  //set and send value
  if(!value_ptr)return;

  m_setValue=v;
  int elapsed=setValueTime.elapsed();
  setValueTime.start();
  if(elapsed<setValueDelayTimer.interval()){
    if(!setValueDelayTimer.isActive())setValueDelayTimer.start();
    //update for displays
    //qDebug()<<elapsed;
    mvar->set_data(m_varmsk,m_type,value_ptr,v);
    setValue_done(v);
    return;
  }else setValueDelayTimer.stop();
  setValue_do(v);
}
void QMandalaField::setValue_do(double v)
{
  _var_float prev_v=value();
  mvar->set_data(m_varmsk,m_type,value_ptr,v);
  switch(m_type){
    case vt_flag:
    case vt_float:
    case vt_vect:
    case vt_point:{
      QByteArray ba;
      ba.resize(32);
      ba.resize(mvar->pack_set((uint8_t*)ba.data(),m_varmsk));
      mvar->send(idx_set,ba);
    }break;
    default:
      mvar->send(m_varmsk&0xFF);
  }
  //restore value for some vars
  if(isSkipWriteField && Datalink::instance()->online()){
    mvar->set_data(m_varmsk,m_type,value_ptr,prev_v);
  }
  setValue_done(v);
}
void QMandalaField::setValue_done(double v)
{
  updateValue(v);
  updateCnt=0;
  if(!isSkipWriteField)setValueUpdateTimer.start();
}
void QMandalaField::setValueLocal(double v)
{
  if(!value_ptr)return;
  mvar->set_data(m_varmsk,m_type,value_ptr,v);
  update();
}
void QMandalaField::setValueDelayTimeout()
{
  //if(m_value==m_setValue)return;
  setValue_do(m_setValue);
}
void QMandalaField::request(void)
{
  if(!mvar)return;
  mvar->request(m_varmsk&0xFF);
}
void QMandalaField::send(void)
{
  if(!mvar)return;
  mvar->send(m_varmsk&0xFF);
}
//=============================================================================
