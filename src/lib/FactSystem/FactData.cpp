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
FactData::FactData(FactTree *parent, QString name, QString title, QString descr, ItemType treeItemType, DataType dataType)
 : FactTree(parent,treeItemType),
   _bindedFact(NULL),
   m_dataType(dataType),
   m_name(name),m_title(title),m_descr(descr)
{
  setObjectName(m_name);
  switch(dataType){
    case EnumData:
      connect(this,&FactData::enumStringsChanged,this,&FactData::textChanged);
    break;
    case BoolData:
      m_value=false;
    break;
    default: break;
  }
  if(parent && m_name.contains('#')){
    connect(parent,&FactData::structChanged,this,&FactData::nameChanged);
  }
  if((treeItemType==GroupItem || treeItemType==SectionItem) && m_dataType==ConstData){
    connect(this,&FactData::sizeChanged,this,&FactData::valueChanged);
    connect(this,&FactData::valueChanged,this,&FactData::textChanged);
  }
}
//=============================================================================
QVariant FactData::value(void) const
{
  if(_bindedFact)return _bindedFact->value();
  switch(m_dataType){
    case ConstData: {
      int sz=size();
      if(m_value.isNull())return sz>0?sz:QVariant();
      break;
    }
    default: break;

  }
  return m_value;
}
//=============================================================================
bool FactData::setValue(const QVariant &v)
{
  if(_bindedFact) return _bindedFact->setValue(v);
  QVariant vx=v;
  if(m_treeItemType==FactItem){
    switch(dataType()){
      case EnumData: {
        if(m_enumStrings.size()<=0)break;
        //list or enum
        QString s=v.toString();
        int idx=m_enumStrings.indexOf(s);
        if(idx<0){
          bool ok=false;
          uint i=s.toUInt(&ok);
          if(ok && (int)i<m_enumStrings.size())idx=i;
        }
        if(idx<0)return false;
        vx=idx;
        break;
      }
      case BoolData: {
        QString s=v.toString().toLower();
        vx=((s=="true"||s=="1"||s=="on"||s=="yes")||v.toUInt()>0)?true:false;
      }
      default: break;
    }
  }
  if(m_value==vx)return false;
  m_value=vx;
  emit valueChanged();
  emit textChanged();
  return true;
}
//=============================================================================
//=============================================================================
FactData::DataType FactData::dataType() const
{
  return m_dataType;
}
QString FactData::name(void) const
{
  return m_name.contains('#')?QString(m_name).replace('#',QString::number(num())):m_name;
}
void FactData::setName(const QString &v)
{
  if(m_name==v)return;
  m_name=v;
  emit nameChanged();
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
  if(treeItemType()==ConstItem){
    if(value().isNull())return name();
    return value().toString();
  }
  if((treeItemType()==GroupItem||treeItemType()==SectionItem) && m_dataType==ConstData && (value().isNull() && (!_bindedFact))){
    return size()>0?QString::number(size()):QString();
  }
  if(treeItemType()==FactItem && dataType()==EnumData && m_enumStrings.size()>0){
    int i=value().toInt();
    if(i<m_enumStrings.size())return m_enumStrings.at(i);
  }
  return value().toString();
}
void FactData::setText(const QString &v)
{
  setValue(v);
}
QStringList FactData::enumStrings() const
{
  return m_enumStrings;
}
void FactData::setEnumStrings(const QStringList &v)
{
  if(m_enumStrings==v)return;
  m_enumStrings=v;
  emit enumStringsChanged();
}
//=============================================================================
FactData * FactData::child(const QString &name) const
{
  foreach(FactTree *item,childItems()){
    if(static_cast<FactData*>(item)->name()==name)return static_cast<FactData*>(item);
  }
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
void FactData::bindValue(FactData *item)
{
  _bindedFact=item;
  connect(item,&FactData::valueChanged,this,&FactData::valueChanged);
  connect(item,&FactData::textChanged,this,&FactData::textChanged);
}
//=============================================================================
void FactData::insertItem(int i, FactTree *item)
{
  FactTree::insertItem(i,item);
  connect(static_cast<FactData*>(item),&FactData::valueChanged,this,&FactData::childValueChanged);
  connect(static_cast<FactData*>(item),&FactData::childValueChanged,this,&FactData::childValueChanged);
}
void FactData::removeItem(FactTree *item)
{
  disconnect(static_cast<FactData*>(item),&FactData::valueChanged,this,&FactData::childValueChanged);
  FactTree::removeItem(item);
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
