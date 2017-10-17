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
 : FactTree(parent,treeItemType),m_dataType(dataType),
   m_name(name),m_title(title),m_descr(descr),
   m_enabled(true), m_visible(true)
{
  setObjectName(m_name);
  if(dataType==EnumData){
    connect(this,&FactData::structChanged,this,&FactData::textChanged);
  }
}
//=============================================================================
QVariant FactData::value(void) const
{
  return m_value;
}
//=============================================================================
bool FactData::setValue(const QVariant &v)
{
  QVariant vx=v;
  if(m_treeItemType==FactItem && dataType()==EnumData && size()>0){
    //list or enum
    FactTree *item=this;
    QString s=v.toString();
    bool ok=false;
    uint i=s.toUInt(&ok);
    if(ok && (int)i<size()) item=FactTree::child(i);
    else item=child(v.toString());
    //qDebug()<<s<<item->name();
    if(item!=this)vx=item->num();
    else return false;
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
  return m_name;
}
QString FactData::title(void) const
{
  return m_title.isEmpty()?m_name:m_title;
}
QString FactData::descr(void) const
{
  return m_descr;
}
bool FactData::enabled() const
{
  return m_enabled;
}
void FactData::setEnabled(const bool &v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged();
}
bool FactData::visible() const
{
  return m_visible;
}
void FactData::setVisible(const bool &v)
{
  if(m_visible==v)return;
  m_visible=v;
  emit visibleChanged();
}
QString FactData::section() const
{
  return m_section;
}
void FactData::setSection(const QString &v)
{
  if(m_section==v)return;
  m_section=v;
  emit sectionChanged();
}
QString FactData::text() const
{
  FactData *item=valueEnumItem();
  if(item) return item->name();
  return value().toString();
}
void FactData::setText(const QString &v)
{
  setValue(v);
}
//=============================================================================
FactData * FactData::child(const QString &name) const
{
  foreach(FactTree *item,m_items){
    if(static_cast<FactData*>(item)->name()==name)return static_cast<FactData*>(item);
  }
  return const_cast<FactData*>(this);
}
QString FactData::path(int fromLevel, const QChar pathDelimiter) const
{
  QString s=name();
  for(const FactData *i=static_cast<FactData*>(parentItem());i && i->level()>=fromLevel;i=static_cast<FactData*>(i->parentItem())){
    s.prepend(i->name()+pathDelimiter);
    if(i->treeItemType()==RootItem && fromLevel>=0)break;
  }
  return s;
}
FactData * FactData::valueEnumItem() const
{
  if(treeItemType()==FactItem && dataType()==EnumData && size()>0){
    //list or enum
    uint n=value().toUInt();
    if((int)n<size()){
      return static_cast<FactData*>(FactTree::child(n));
    }
  }
  return NULL;
}
//=============================================================================
void FactData::copyValuesFrom(const FactData *item)
{
  foreach(FactTree *i,m_items){
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
  connect(this,&FactData::valueChanged,this,&FactData::updateBindedValue);
  connect(item,&FactData::valueChanged,this,&FactData::bindedValueChanged);
}
void FactData::updateBindedValue()
{
  _bindedFact->setValue(value());
}
void FactData::bindedValueChanged()
{
  setValue(static_cast<FactData*>(sender())->value());
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
  if (index.row() < 0 || index.row() >= m_items.count())
    return QVariant();
  FactData *item=static_cast<FactData*>(m_items[index.row()]);
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
  if (index.row() < 0 || index.row() >= m_items.count() || role != Qt::EditRole)
    return false;
  static_cast<FactData*>(m_items[index.row()])->setValue(value);
  return true;
}
//=============================================================================
