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
   m_title(title),m_descr(descr)
{
  setObjectName(m_name);
  setDataType(dataType);
  connect(this,&FactData::enumStringsChanged,this,&FactData::textChanged);
  if(parent && m_name.contains('#')){
    connect(parent,&FactData::structChanged,this,&FactData::nameChanged);
  }
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
  if(m_treeItemType==FactItem){
    switch(dataType()){
      case EnumData:
        if(ev<0)return false;
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
        else if(!m_enumStrings.isEmpty())vx=QVariant();
      break;
      default:
        if(ev>=0)vx=ev;
      break;
    }
  }
  if(m_value==vx)return false;
  m_value=vx;
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
//=============================================================================
FactData::DataType FactData::dataType() const
{
  return m_dataType;
}
void FactData::setDataType(const DataType &v)
{
  if(m_dataType==v)return;
  m_dataType=v;
  //default data
  switch(m_dataType){
    case FloatData: m_value=0.0; break;
    case IntData: m_value=0; break;
    case BoolData: m_value=false; break;
    case EnumData: m_value=0; break;
    default: m_value=QVariant();
  }
  emit dataTypeChanged();
  emit valueChanged();
  emit textChanged();
}
QString FactData::title(void) const
{
  return m_title.isEmpty()?m_name:m_title;
}
void FactData::setTitle(const QString &v)
{
  if(m_title==v)return;
  m_title=v;
  emit titleChanged();
  //emit dataChanged(QModelIndex(nu
}
QString FactData::descr(void) const
{
  return m_descr;
}
void FactData::setDescr(const QString &v)
{
  if(m_descr==v)return;
  m_descr=v;
  emit descrChanged();
}
QString FactData::text() const
{
  if(_bindedFact) return _bindedFact->text();
  if(treeItemType()==FactItem && (!m_enumStrings.isEmpty())){
    int ev=enumValue(value());
    if(ev>=0)return enumText(ev);
    if(m_dataType==IntData)return QString();
  }
  return value().toString();
}
void FactData::setText(const QString &v)
{
  if(_bindedFact){
    _bindedFact->setText(v);
    return;
  }
  setValue(v);
}
QStringList FactData::enumStrings() const
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
  foreach(FactTree *i,childItemsTree()){
    FactData *childFact=static_cast<FactData*>(i);
    foreach(FactTree *i2,item->childItemsTree()){
      FactData *itemChildFact=static_cast<FactData*>(i2);
      if(childFact->name()!=itemChildFact->name())continue;
      childFact->bind(itemChildFact);
    }
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
// LIST MODEL
//=============================================================================
QHash<int, QByteArray> FactData::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[ModelDataRole]  = "modelData";
  roles[NameRole]       = "name";
  roles[ValueRole]      = "value";
  roles[TextRole]       = "text";
  return roles;
}
QVariant FactData::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_UNUSED(orientation)
  if(role==Qt::DisplayRole){
    switch(section){
      case FACT_ITEM_COLUMN_NAME: return tr("Name");
      case FACT_ITEM_COLUMN_VALUE: return tr("Value");
      case FACT_ITEM_COLUMN_DESCR: return tr("Description");
    }
  }
  return QVariant();
}
Qt::ItemFlags FactData::flags(const QModelIndex &index) const
{
  Qt::ItemFlags f=Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  if(m_treeItemType==FactItem){
    f|=Qt::ItemNeverHasChildren;
    if(index.column()==FACT_ITEM_COLUMN_VALUE) f|=Qt::ItemIsEditable;
  }
  return f;
}
QVariant FactData::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= size())
    return QVariant();
  FactData *item=static_cast<FactData*>(childItems().at(index.row()));
  switch(role){
    case ModelDataRole: return QVariant::fromValue(item);
    case NameRole:      return item->name();
    case ValueRole:     return item->value();
    case TextRole:      return item->text();
  }
  return QVariant();
}
bool FactData::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (index.row() < 0 || index.row() >= size() || role != Qt::EditRole)
    return false;
  static_cast<FactData*>(childItems().at(index.row()))->setValue(value);
  return true;
}
//=============================================================================
