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
#include "FactData.h"
//=============================================================================
FactData::FactData(FactTree *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType)
 : FactTree(parent,name,treeItemType),
   _bindedFact(NULL),
   m_dataType(NoData),
   m_modified(false),
   m_precision(-1),
   m_title(title),m_descr(descr)
{
  m_dataType=dataType;
  backup_set=false;
  defaults();
  connect(this,&FactData::enumStringsChanged,this,&FactData::textChanged);
}
//=============================================================================
QVariant FactData::value(void) const
{
  if(_bindedFact)return _bindedFact->value();
  return m_value;
}
//=============================================================================
bool FactData::setValue(const QVariant &v)
{
  if(_bindedFact) return _bindedFact->setValue(v);
  QVariant vx=v;
  int ev=enumValue(v);
  bool ok=false;
  if(m_treeItemType==FactItem){
    switch(dataType()){
      case EnumData:
        if(ev<0){
          if(m_enumStrings.size()==2){
            //try boolean strings
            QString s=v.toString();
            if(s=="true"||s=="on"||s=="yes")ev=enumValue(1);
            else if(s=="false"||s=="off"||s=="no")ev=enumValue(0);
            if(ev<0){
              if(m_value.isNull())ev=0;
              else return false;
            }
          }else if(m_value.isNull())ev=0;
          else return false;
        }
        vx=ev;
      break;
      case BoolData:
        if(ev<0){
          QString s=v.toString().toLower();
          vx=((s=="true"||s=="1"||s=="on"||s=="yes")||v.toUInt()>0)?true:false;
        }else vx=ev;
      break;
      case TextData:
        if(ev>=0)vx=enumText(ev);
      break;
      case IntData:
        if(ev>=0)vx=ev;
        else if(!m_enumStrings.isEmpty())return false;
        else if(v.type()!=QVariant::Int){
          QString s=v.toString();
          int i=s.toInt(&ok);
          if(!ok)i=s.toInt(&ok,16);
          if(!ok) return false;
          if((!m_min.isNull()) && i<m_min.toInt())i=m_min.toInt();
          if((!m_max.isNull()) && i>m_max.toInt())i=m_max.toInt();
          vx=i;
        }else{
          int i=v.toInt();
          if((!m_min.isNull()) && i<m_min.toInt())i=m_min.toInt();
          if((!m_max.isNull()) && i>m_max.toInt())i=m_max.toInt();
          vx=i;
        }
      break;
      case FloatData:
        if(v.type()!=QVariant::Double){
          QString s=v.toString();
          double d=s.toDouble(&ok);
          if(!ok){
            //try boolean
            if(s=="true"||s=="on"||s=="yes")d=1;
            else if(s=="false"||s=="off"||s=="no")d=0;
            else return false;
          }
          if((!m_min.isNull()) && d<m_min.toInt())d=m_min.toInt();
          if((!m_max.isNull()) && d>m_max.toInt())d=m_max.toInt();
          vx=d;
        }
        vx=v.toDouble();
      break;
      case MandalaData:
        vx=stringToMandala(v.toString());
      break;
      case ScriptData:
        vx=v.toString();
      break;
      default:
        if(ev>=0)vx=ev;
      break;
    }
  }
  if(m_value==vx)return false;
  if(dataType()==FloatData){
    QVariant vbak=m_value;
    QString sv=text();
    m_value=vx;
    if(sv==text()){
      m_value=vbak;
      return false;
    }
  }else{
    m_value=vx;
  }

  if(backup_set)
    setModified(backup_value!=m_value);
  emit valueChanged();
  emit textChanged();
  return true;
}
//=============================================================================
int FactData::enumValue(const QVariant &v) const
{
  if(m_enumStrings.isEmpty())return -1;
  QString s=v.toString();
  int idx=m_enumStrings.indexOf(s);
  if(idx>=0){
    if(!m_enumValues.isEmpty())idx=m_enumValues.at(idx);
  }else{
    bool ok=false;
    int i=s.toInt(&ok);
    if(ok){
      if(!m_enumValues.isEmpty()){
        if(m_enumValues.contains(i))idx=i;
      }else if(i>=0 && i<m_enumStrings.size())idx=i;
    }
  }
  return idx;
}
QString FactData::enumText(int v) const
{
  if(!m_enumValues.isEmpty()){
    if(m_enumValues.contains(v))return m_enumStrings.at(m_enumValues.indexOf(v));
  }else{
    if(v>=0 && v<m_enumStrings.size())return m_enumStrings.at(v);
  }
  return QString();
}
//=============================================================================
bool FactData::isZero() const
{
  if(m_treeItemType==GroupItem || m_treeItemType==SectionItem){
    for (int i=0;i<childItems().size();i++) {
      FactData *f=static_cast<FactData*>(childItems().at(i));
      if(!f->isZero()) return false;
    }
    return true;
  }
  if(dataType()==TextData) return text().isEmpty();
  if(dataType()==ScriptData) return value().toString().isEmpty();
  const QString &s=text();
  if(s.isEmpty())return true;
  /*if(dataType()==EnumData){
    return m_value.toInt()==0 && (s=="off"||s=="default"||s=="auto"||s==QVariant::fromValue(false).toString());
  }*/
  if(s=="0")return true;
  if(dataType()==FloatData)return m_value.toDouble()==0.0;
  if(m_value.toInt()==0)return true;
  return false;
}
//=============================================================================
//=============================================================================
FactData::DataType FactData::dataType() const
{
  return m_dataType;
}
void FactData::setDataType(const DataType &v)
{
  if(m_dataType==v)return;
  m_dataType=v;
  defaults();
  emit dataTypeChanged();
  emit valueChanged();
  emit textChanged();
}
bool FactData::modified() const
{
  return m_modified;
}
void FactData::setModified(const bool &v)
{
  if(m_modified==v)return;
  m_modified=v;
  emit modifiedChanged();
}
int FactData::precision(void) const
{
  return m_precision;
}
void FactData::setPrecision(const int &v)
{
  if(m_precision==v)return;
  m_precision=v;
  emit precisionChanged();
  emit textChanged();
}
QVariant FactData::min(void) const
{
  return m_min;
}
void FactData::setMin(const QVariant &v)
{
  if(m_min==v)return;
  m_min=v;
  emit minChanged();
  if(v.isNull())return;
  if(value()<m_min)setValue(m_min);
}
QVariant FactData::max(void) const
{
  return m_max;
}
void FactData::setMax(const QVariant &v)
{
  if(m_max==v)return;
  m_max=v;
  emit maxChanged();
  if(v.isNull())return;
  if(value()>m_max)setValue(m_max);
}
QString FactData::title(void) const
{
  return m_title.isEmpty()?name():m_title;
}
void FactData::setTitle(const QString &v)
{
  QString s=v.trimmed();
  if(m_title==s)return;
  m_title=s;
  emit titleChanged();
  //emit dataChanged(QModelIndex(nu
}
QString FactData::descr(void) const
{
  return m_descr;
}
void FactData::setDescr(const QString &v)
{
  QString s=v.trimmed();
  if(m_descr==s)return;
  m_descr=s;
  emit descrChanged();
}
QString FactData::text() const
{
  if(_bindedFact) return _bindedFact->text();
  const QVariant &v=value();
  if(treeItemType()==FactItem && (!m_enumStrings.isEmpty())){
    int ev=enumValue(v);
    if(ev>=0)return enumText(ev);
    if(m_dataType==IntData)return QString();
  }
  if(m_dataType==IntData){
    if(units()=="hex")return QString::number(v.toUInt(),16).toUpper();
    return QString::number(v.toInt());
  }
  if(m_dataType==MandalaData){
    return mandalaToString(v.toUInt());
  }
  if(v.type()==QVariant::ByteArray) return v.toByteArray().toHex().toUpper();
  if(v.type()==QVariant::Double){
    double vf=v.toDouble();
    if(vf==0.0)return "0";
    QString s=QString("%1").arg(vf,0,'f',m_precision>0?m_precision:8);
    if(s.contains('.')){
      if(m_precision>0)s=s.left(s.indexOf('.')+1+m_precision);
      while(s.endsWith('0'))s.chop(1);
      if(s.endsWith('.'))s.chop(1);
    }
    return s;
    //return QString::asprintf("%f").arg(vf,0,'g',m_precision);
  }
  return v.toString();
}
const QStringList & FactData::enumStrings() const
{
  if(_bindedFact) return _bindedFact->enumStrings();
  return m_enumStrings;
}
void FactData::setEnumStrings(const QStringList &v, const QList<int> &enumValues)
{
  if(_bindedFact){
    _bindedFact->setEnumStrings(v,enumValues);
    return;
  }
  if(m_enumStrings==v)return;
  m_enumStrings=v;
  if(v.size()==enumValues.size()) m_enumValues=enumValues;
  else m_enumValues.clear();
  emit enumStringsChanged();
}
void FactData::setEnumStrings(const QMetaEnum &v)
{
  QStringList st;
  QList<int> vlist;
  for(int i=0;i<v.keyCount();++i){
    int vi=v.value(i);
    st.append(v.valueToKey(vi));
    vlist.append(vi);
  }
  setEnumStrings(st,vlist);
}
QString FactData::units() const
{
  return m_units;
}
void FactData::setUnits(const QString &v)
{
  if(m_units==v)return;
  m_units=v;
  emit unitsChanged();
}
QVariant FactData::defaultValue(void) const
{
  return m_defaultValue;
}
void FactData::setDefaultValue(const QVariant &v)
{
  if(m_defaultValue==v)return;
  m_defaultValue=v;
  emit defaultValueChanged();
}
//=============================================================================
QString FactData::mandalaToString(quint16 mid) const
{
  return QString::number(mid);
}
quint16 FactData::stringToMandala(const QString &s) const
{
  return s.toUInt();
}
const QStringList * FactData::mandalaNames() const
{
  return NULL;
}
//=============================================================================
void FactData::copyValuesFrom(const FactData *item)
{
  foreach(FactTree *i,childItems()){
    FactData *dest=static_cast<FactData*>(i);
    i=item->child(dest->name());
    if(!i)continue;
    FactData *src=static_cast<FactData*>(i);
    if(dest->treeItemType()!=src->treeItemType() || dest->dataType()!=src->dataType())
      continue;
    if(dest->treeItemType()==FactItem){
      dest->setValue(src->value());
    }else if(dest->treeItemType()==GroupItem){
      dest->copyValuesFrom(src);
    }
  }
}
//=============================================================================
void FactData::bind(FactData *item)
{
  if(_bindedFact){
    disconnect(_bindedFact,&FactData::valueChanged,this,&FactData::valueChanged);
    disconnect(_bindedFact,&FactData::textChanged,this,&FactData::textChanged);
    disconnect(_bindedFact,&FactData::enumStringsChanged,this,&FactData::enumStringsChanged);
  }
  if(item==this)return;
  _bindedFact=item;
  connect(item,&FactData::valueChanged,this,&FactData::valueChanged);
  connect(item,&FactData::textChanged,this,&FactData::textChanged);
  connect(item,&FactData::enumStringsChanged,this,&FactData::enumStringsChanged);
  emit valueChanged();
  emit textChanged();
  emit enumStringsChanged();
  //bind all matched children
  foreach(FactTree *i,childItems()){
    FactData *childFact=static_cast<FactData*>(i);
    foreach(FactTree *i2,item->childItems()){
      FactData *itemChildFact=static_cast<FactData*>(i2);
      if(childFact->name()!=itemChildFact->name())continue;
      childFact->bind(itemChildFact);
    }
  }
}
//=============================================================================
void FactData::backup()
{
  if(size()){
    foreach (FactTree *i, childItems()) {
      static_cast<FactData*>(i)->backup();
    }
    return;
  }
  if(treeItemType()!=FactItem)return;
  backup_value=m_value;
  backup_set=true;
  setModified(false);
}
void FactData::restore()
{
  if(!modified())return;
  if(size()){
    foreach (FactTree *i, childItems()) {
      static_cast<FactData*>(i)->restore();
    }
    return;
  }
  if(treeItemType()!=FactItem)return;
  setValue(backup_value);
  setModified(false);
}
void FactData::defaults()
{
  switch(m_dataType){
    case FloatData: m_value=0.0; break;
    case IntData: m_value=0; break;
    case BoolData: m_value=false; break;
    case EnumData: m_value=0; break;
    case MandalaData: m_value=0; break;
    default: m_value=QVariant();
  }
}
//=============================================================================
void FactData::insertItem(int i, FactTree *item)
{
  FactTree::insertItem(i,item);
  connect(static_cast<FactData*>(item),&FactData::valueChanged,this,&FactData::childValueChanged);
  connect(static_cast<FactData*>(item),&FactData::childValueChanged,this,&FactData::childValueChanged);
}
void FactData::removeItem(FactTree *item, bool deleteLater)
{
  disconnect(static_cast<FactData*>(item),&FactData::valueChanged,this,&FactData::childValueChanged);
  FactTree::removeItem(item,deleteLater);
}
//=============================================================================
