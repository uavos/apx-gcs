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
#include "AppPropertiesTree.h"
//=============================================================================
//root mandala
AppPropertiesTree::AppPropertiesTree(QObject *parent, QString name, QString descr)
 : QAbstractListModel(parent),m_parentItem(NULL),m_level(0),
   m_name(name),m_descr(descr)
{
  setObjectName(m_name);
}
//=============================================================================
//group or class parent of field
AppPropertiesTree::AppPropertiesTree(AppPropertiesTree *parent, QString name, QString descr, QString alias)
 : QAbstractListModel(parent),m_parentItem(parent),m_level(0),
   m_name(name),m_descr(descr),m_alias(alias)
{
  setObjectName(m_name);
  parent->addItem(this);

  //find tree item type
  for(AppPropertiesTree *item=m_parentItem;item;item=item->m_parentItem){
    m_level++;
  }

  emit structChanged(this);
}
//=============================================================================
AppPropertiesTree::~AppPropertiesTree()
{
  if(m_parentItem){
    m_parentItem->m_items.removeAll(this);
  }
}
//=============================================================================
void AppPropertiesTree::addItem(AppPropertiesTree *item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  m_items << item;
  item->m_parentItem=this;
  connect(item,&AppPropertiesTree::valueChanged,this,&AppPropertiesTree::valueChanged);
  connect(item,&AppPropertiesTree::structChanged,this,&AppPropertiesTree::structChanged);
  setProperty(item->name().toUtf8().data(),qVariantFromValue(item));
  endInsertRows();
  emit sizeChanged(this);
}
void AppPropertiesTree::removeItem(AppPropertiesTree *item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  m_items << item;
  item->m_parentItem=this;
  connect(item,&AppPropertiesTree::valueChanged,this,&AppPropertiesTree::valueChanged);
  connect(item,&AppPropertiesTree::structChanged,this,&AppPropertiesTree::structChanged);
  setProperty(item->name().toUtf8().data(),qVariantFromValue(item));
  endInsertRows();
  emit sizeChanged(this);
}
//=============================================================================
void AppPropertiesTree::clear(void)
{
  foreach(AppPropertiesTree *i,m_items){
    i->clear();
  }
  qDeleteAll(m_items);
  m_items.clear();
}
//=============================================================================
void AppPropertiesTree::reset(void)
{
  foreach(AppPropertiesTree *i,m_items){
    i->reset();
  }
  setValue(QVariant());
}
//=============================================================================
QVariant AppPropertiesTree::value(void) const
{
  return m_value;
}
//=============================================================================
bool AppPropertiesTree::setValue(const QVariant &v)
{
  if(m_value==v)return false;
  m_value=v;
  emit valueChanged(this);
  return true;
}
//=============================================================================
int AppPropertiesTree::num() const
{
  if(!m_parentItem) return 0;
  return m_parentItem->m_items.indexOf(const_cast<AppPropertiesTree*>(this));
}
//=============================================================================
//=============================================================================
QString AppPropertiesTree::name(void) const
{
  return m_name;
}
QString AppPropertiesTree::descr(void) const
{
  return m_descr;
}
QString AppPropertiesTree::alias(void) const
{
  return m_alias;
}
int AppPropertiesTree::level(void) const
{
  return m_level;
}
QString AppPropertiesTree::valueText(void) const
{
  return value().toString();
}
int AppPropertiesTree::size() const
{
  return m_items.size();
}
//=============================================================================
bool AppPropertiesTree::isField(void) const
{
  return m_items.isEmpty();
}
bool AppPropertiesTree::isFieldsGroup(void) const
{
  return (!m_items.isEmpty())&&m_items.first()->isField();
}
bool AppPropertiesTree::isRoot(void) const
{
  return (!m_parentItem);
}
//=============================================================================
AppPropertiesTree *AppPropertiesTree::parentItem() const
{
  return m_parentItem;
}
AppPropertiesTree *AppPropertiesTree::child(int n)
{
  if(n>=m_items.size())return NULL;
  return m_items.at(n);
}
//=============================================================================
AppPropertiesTree * AppPropertiesTree::childByName(const QString &itemName) const
{
  foreach(AppPropertiesTree *item,m_items){
    if(item->name()==itemName)return item;
  }
  return const_cast<AppPropertiesTree*>(this);
}
AppPropertiesTree * AppPropertiesTree::find(const QString &itemNamePath) const
{
  QString s=itemNamePath;
  AppPropertiesTree *item=const_cast<AppPropertiesTree*>(this);
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
AppPropertiesTree * AppPropertiesTree::findByAlias(const QString &itemAlias) const
{
  foreach(AppPropertiesTree *i,m_items){
    if(i->alias()==itemAlias)return i;
    i=i->findByAlias(itemAlias);
    if(i)return i;
  }
  return NULL;
}
QString AppPropertiesTree::path(int fromLevel) const
{
  QString s=name();
  for(const AppPropertiesTree *i=parentItem();i && i->level()>=fromLevel;i=i->parentItem()){
    s.prepend(i->name()+".");
  }
  return s;
}
QList<AppPropertiesTree *> AppPropertiesTree::allFields() const
{
  QList<AppPropertiesTree*> list;
  foreach(AppPropertiesTree *i,m_items){
    if(i->isField()){
      list.append(static_cast<AppPropertiesTree*>(i));
    }else{
      list.append(i->allFields());
    }
  }
  return list;
}
//=============================================================================
// LIST MODEL
//=============================================================================
int AppPropertiesTree::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  return size();
}
QHash<int, QByteArray> AppPropertiesTree::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[ItemRole]   = "item";
  roles[NameRole]   = "name";
  roles[ValueRole]  = "value";
  roles[DescrRole]  = "descr";
  return roles;
}
QVariant AppPropertiesTree::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_UNUSED(orientation)
  if(role==Qt::DisplayRole){
    switch(section){
      case MANDALA_ITEM_COLUMN_NAME: return tr("Name");
      case MANDALA_ITEM_COLUMN_VALUE: return tr("Value");
      case MANDALA_ITEM_COLUMN_DESCR: return tr("Description");
    }
  }
  return QVariant();
}
Qt::ItemFlags AppPropertiesTree::flags(const QModelIndex &index) const
{
  Qt::ItemFlags f=Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  if(isField()){
    f|=Qt::ItemNeverHasChildren;
    if(index.column()==MANDALA_ITEM_COLUMN_VALUE) f|=Qt::ItemIsEditable;
  }
  return f;
}
QVariant AppPropertiesTree::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= m_items.count())
    return QVariant();
  AppPropertiesTree *item=m_items[index.row()];
  switch(role){
    case ItemRole: return QVariant::fromValue(item);
    case NameRole: return item->name();
    case ValueRole: return item->value();
    case DescrRole: return item->descr();
  }
  return QVariant();
}
bool AppPropertiesTree::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (index.row() < 0 || index.row() >= m_items.count() || role != Qt::EditRole)
    return false;
  m_items[index.row()]->setValue(value);
  return true;
}
//=============================================================================
//=============================================================================
