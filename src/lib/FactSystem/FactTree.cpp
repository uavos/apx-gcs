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
#include "FactTree.h"
//=============================================================================
//root mandala
FactTree::FactTree(QObject *parent, QString name, QString descr)
 : QAbstractListModel(parent),m_parentItem(NULL),m_level(0),
   m_name(name),m_descr(descr)
{
  setObjectName(m_name);
  connect(this,&FactTree::structChanged,this,&FactTree::sizeChanged);
}
//=============================================================================
//group or class parent of field
FactTree::FactTree(FactTree *parent, QString name, QString descr, QString alias)
 : QAbstractListModel(parent),m_parentItem(parent),m_level(0),
   m_name(name),m_descr(descr),m_alias(alias)
{
  setObjectName(m_name);
  parent->addItem(this);

  //find tree item type
  for(FactTree *item=m_parentItem;item;item=item->m_parentItem){
    m_level++;
  }

  emit structChanged(this);
  connect(this,&FactTree::structChanged,this,&FactTree::sizeChanged);
}
//=============================================================================
FactTree::~FactTree()
{
  if(m_parentItem){
    m_parentItem->removeItem(this);
  }
}
//=============================================================================
void FactTree::addItem(FactTree *item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  m_items << item;
  item->m_parentItem=this;
  connect(item,&FactTree::valueChanged,this,&FactTree::valueChanged);
  connect(item,&FactTree::structChanged,this,&FactTree::structChanged);
  setProperty(item->name().toUtf8().data(),qVariantFromValue(item));
  endInsertRows();
  emit structChanged(this);
}
void FactTree::removeItem(FactTree *item)
{
  int i=m_items.indexOf(item);
  if(i<0)return;
  beginRemoveRows(QModelIndex(), i, i);
  m_items.removeOne(item);
  item->m_parentItem=NULL;
  endRemoveRows();
  disconnect(item,&FactTree::valueChanged,this,&FactTree::valueChanged);
  disconnect(item,&FactTree::structChanged,this,&FactTree::structChanged);
  setProperty(item->name().toUtf8().data(),QVariant());
  item->deleteLater();
  emit structChanged(this);
}
//=============================================================================
void FactTree::clear(void)
{
  foreach(FactTree *i,m_items){
    i->clear();
  }
  beginRemoveRows(QModelIndex(), 0, m_items.size());
  foreach (FactTree *item, m_items) {
    item->m_parentItem=NULL;
    disconnect(item,&FactTree::valueChanged,this,&FactTree::valueChanged);
    disconnect(item,&FactTree::structChanged,this,&FactTree::structChanged);
  }
  qDeleteAll(m_items);
  m_items.clear();
  endRemoveRows();
  emit structChanged(this);
}
//=============================================================================
void FactTree::reset(void)
{
  foreach(FactTree *i,m_items){
    i->reset();
  }
  setValue(QVariant());
}
//=============================================================================
QVariant FactTree::value(void) const
{
  return m_value;
}
//=============================================================================
bool FactTree::setValue(const QVariant &v)
{
  if(m_value==v)return false;
  m_value=v;
  emit valueChanged(this);
  return true;
}
//=============================================================================
int FactTree::num() const
{
  if(!m_parentItem) return 0;
  return m_parentItem->m_items.indexOf(const_cast<FactTree*>(this));
}
//=============================================================================
//=============================================================================
QString FactTree::name(void) const
{
  return m_name;
}
QString FactTree::descr(void) const
{
  return m_descr;
}
QString FactTree::alias(void) const
{
  return m_alias;
}
int FactTree::level(void) const
{
  return m_level;
}
QString FactTree::valueText(void) const
{
  return value().toString();
}
int FactTree::size() const
{
  return m_items.size();
}
//=============================================================================
bool FactTree::isField(void) const
{
  return m_items.isEmpty();
}
bool FactTree::isFieldsGroup(void) const
{
  return (!m_items.isEmpty())&&m_items.first()->isField();
}
bool FactTree::isRoot(void) const
{
  return (!m_parentItem);
}
//=============================================================================
FactTree *FactTree::parentItem() const
{
  return m_parentItem;
}
FactTree *FactTree::child(int n)
{
  if(n>=m_items.size())return NULL;
  return m_items.at(n);
}
//=============================================================================
FactTree * FactTree::childByName(const QString &itemName) const
{
  foreach(FactTree *item,m_items){
    if(item->name()==itemName)return item;
  }
  return const_cast<FactTree*>(this);
}
FactTree * FactTree::find(const QString &itemNamePath) const
{
  QString s=itemNamePath;
  FactTree *item=const_cast<FactTree*>(this);
  while(!s.isEmpty()){
    if(item->isFieldsGroup()){
      item=item->childByName(s);
      break;
    }else{
      int i=s.indexOf('.');
      if(i<=0)break;
      item=item->childByName(s.left(i));
      s.remove(0,i+1);
    }
  }
  return item;
}
FactTree * FactTree::findByAlias(const QString &itemAlias) const
{
  foreach(FactTree *i,m_items){
    if(i->alias()==itemAlias)return i;
    i=i->findByAlias(itemAlias);
    if(i)return i;
  }
  return NULL;
}
QString FactTree::path(int fromLevel) const
{
  QString s=name();
  for(const FactTree *i=parentItem();i && i->level()>=fromLevel;i=i->parentItem()){
    s.prepend(i->name()+".");
  }
  return s;
}
QList<FactTree *> FactTree::allFields() const
{
  QList<FactTree*> list;
  foreach(FactTree *i,m_items){
    if(i->isField()){
      list.append(static_cast<FactTree*>(i));
    }else{
      list.append(i->allFields());
    }
  }
  return list;
}
//=============================================================================
// LIST MODEL
//=============================================================================
int FactTree::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  return size();
}
QHash<int, QByteArray> FactTree::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[ItemRole]   = "item";
  roles[NameRole]   = "name";
  roles[ValueRole]  = "value";
  roles[DescrRole]  = "descr";
  roles[ValueTextRole]  = "valueText";
  return roles;
}
QVariant FactTree::headerData(int section, Qt::Orientation orientation, int role) const
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
Qt::ItemFlags FactTree::flags(const QModelIndex &index) const
{
  Qt::ItemFlags f=Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  if(isField()){
    f|=Qt::ItemNeverHasChildren;
    if(index.column()==FACT_ITEM_COLUMN_VALUE) f|=Qt::ItemIsEditable;
  }
  return f;
}
QVariant FactTree::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= m_items.count())
    return QVariant();
  FactTree *item=m_items[index.row()];
  switch(role){
    case ItemRole: return QVariant::fromValue(item);
    case NameRole: return item->name();
    case ValueRole: return item->value();
    case DescrRole: return item->descr();
    case ValueTextRole: return item->value().toString(); // TODO: expand enums
  }
  return QVariant();
}
bool FactTree::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (index.row() < 0 || index.row() >= m_items.count() || role != Qt::EditRole)
    return false;
  m_items[index.row()]->setValue(value);
  return true;
}
//=============================================================================
//=============================================================================
