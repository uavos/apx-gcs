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
#include "MissionItemField.h"
#include "QMandala.h"
#include "FactSystem.h"
//=============================================================================
MissionItemField::MissionItemField(MissionItem *parent, QString name, uint ftype, QStringList opts,QString descr)
 : MissionItem(parent,name),ftype(ftype),opts(opts)
{
  setObjectName(name);
  setDescr(descr);
  connect(this,SIGNAL(changed()),parent,SIGNAL(changed()),Qt::QueuedConnection);
}
//=============================================================================
QVariant MissionItemField::data(int column,int role) const
{
  if(column!=tc_value) return MissionItem::data(column,role);
  QVariant v=value();
  if(role==Qt::DisplayRole){
    switch(ftype){
      case dt_option:   return v.toUInt()<(uint)opts.size()?opts.at(v.toUInt()):opts.first();
      case dt_distance: return v.toUInt()<(uint)opts.size()?opts.at(v.toUInt()):QString("%1 m").arg(v.toInt());
      case dt_angle:    return QString("%1 deg").arg(v.toInt());
      case dt_lat:      return FactSystem::latToString(v.toDouble());
      case dt_lon:      return FactSystem::lonToString(v.toDouble());
      case dt_time:     return v.toUInt()?QTime(0,0).addSecs(v.toUInt()).toString("HH:mm:ss"):QString(tr("off"));
      case dt_varmsk:   return QMandala::instance()->current->field(v.toInt())->name();
      case dt_byte:     return v.toUInt()<(uint)opts.size()?opts.at(v.toUInt()):v.toString();
    }
  }else if(role==Qt::EditRole){
    switch(ftype){
      case dt_lat:      return FactSystem::latToString(v.toDouble());
      case dt_lon:      return FactSystem::lonToString(v.toDouble());
      case dt_time:     return QTime(0,0).addSecs(v.toUInt());
      case dt_option:   return v.toUInt()<(uint)opts.size()?opts.at(v.toUInt()):opts.first();
      case dt_varmsk:   return QMandala::instance()->current->field(v.toInt())->name();
    }
  }
  return MissionItem::data(column,role);
}
//=============================================================================
QVariant MissionItemField::value(void) const
{
  //force field types
  switch(ftype){
    case dt_option:   return m_value.toInt();
    case dt_varmsk:   return m_value.toInt();
    case dt_distance:
    case dt_angle:    return m_value.toInt();
    case dt_lat:
    case dt_lon:      return m_value.toDouble();
    case dt_script:   return m_value.toString();
    case dt_float:    return m_value.toDouble();
    case dt_byte:     return m_value.toUInt();
    case dt_time:     return m_value.toUInt();
  }
  return m_value;
}
//=============================================================================
bool MissionItemField::setValue(QVariant value)
{
  QVariant v=value;
  switch(ftype){
    case dt_lat:
    case dt_lon: {
      bool ok=false;
      double vf=v.toDouble(&ok);
      if(!ok){
        vf=(ftype==dt_lat)?FactSystem::latFromString(v.toString()):FactSystem::lonFromString(v.toString());
      }else vf=QMandala::instance()->current->boundAngle(vf);
      v=vf;
    }break;
    case dt_distance:
    case dt_angle:{
      bool ok=false;
      double iv=v.toDouble(&ok);
      QString vs=v.toString().toLower();
      if(vs.size()&&opts.contains(vs))iv=opts.indexOf(vs);
      else if(!ok) iv=0;
      v=iv;
    }break;
    case dt_option: {
      QString vs=v.toString().toLower();
      if(opts.contains(vs))v=opts.indexOf(vs);
      else if(v.toUInt()>=(uint)opts.size())v=0;
    }break;
    case dt_byte:{
      bool ok=false;
      uint iv=v.toUInt(&ok);
      QString vs=v.toString().toLower();
      if(vs.size()&&opts.contains(vs))iv=opts.indexOf(vs);
      else if(!ok) iv=0;
      v=iv;
    }break;
    case dt_time:     v=v.toUInt(); break;
    case dt_float:    v=v.toDouble(); break;
    case dt_varmsk:   v=QMandala::instance()->current->field(v.toString())->varmsk(); break;
  }
  if(m_value!=v){
    m_value=v;
    emit changed();
  }
  return true;
}
//=============================================================================
void MissionItemField::backup()
{
  m_bkp=m_value;
  emit changed();
}
void MissionItemField::invalidate()
{
  m_bkp=QVariant();
  emit changed();
}
//=============================================================================
bool MissionItemField::isModified(void) const
{
  return m_bkp!=m_value;
}
//=============================================================================
Qt::ItemFlags MissionItemField::flags(int column) const
{
  Qt::ItemFlags f=MissionItem::flags(column)|Qt::ItemNeverHasChildren;
  if(column==tc_value && !childItems.size())
    return f|Qt::ItemIsEditable;
  return f;
}
//=============================================================================
void MissionItemField::restore(void)
{
  m_value=m_bkp;
  emit changed();
}
//=============================================================================
bool MissionItemField::isZero(void) const
{
  if(m_value.isNull())return true;
  switch(ftype){
    case dt_script:   return m_value.toString().isEmpty();
  }
  return m_value.toUInt()==0;
}
//=============================================================================
QByteArray MissionItemField::md5() const
{
  QByteArray ba;
  ba.append(value().toString());
  return QCryptographicHash::hash(ba,QCryptographicHash::Md5);
}
//=============================================================================
void MissionItemField::saveToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  QString s=value().toString();
  if(s.isEmpty())return;
  dom.appendChild(doc.createElement(objectName())).appendChild(doc.createTextNode(s));
}
//=============================================================================
void MissionItemField::loadFromXml(QDomNode dom)
{
  QDomElement e=dom.firstChildElement(objectName()); //find field
  if(e.isNull())return;
  setValue(e.text());
}
//=============================================================================
//=============================================================================
MissionItemFieldGroup::MissionItemFieldGroup(MissionItem *parent,QString name,QString caption,QString descr)
: MissionItem(parent,name,caption,descr)
{
}
//=============================================================================
void MissionItemFieldGroup::saveToXml(QDomNode dom) const
{
  QDomDocument doc=dom.ownerDocument();
  dom=dom.appendChild(doc.createElement(objectName()));
  MissionItem::saveToXml(dom);
}
//=============================================================================
void MissionItemFieldGroup::loadFromXml(QDomNode dom)
{
  QDomElement e=dom.firstChildElement(objectName()); //find field
  if(e.isNull())return;
  MissionItem::loadFromXml(e);
}
//=============================================================================
