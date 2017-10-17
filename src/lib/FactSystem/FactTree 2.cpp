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
//#include <QtQml>
//#include <QQmlEngine>

#include "FactTree.h"
//=============================================================================
// root
FactTree::FactTree(QObject *parent, QString name, QString title, QString descr)
 : QAbstractListModel(parent),m_parentItem(NULL),itemType(RootItem),
   m_level(0),
   m_name(name),m_title(title),m_descr(descr),
   m_enabled(true), m_visible(true)
{
  setObjectName(m_name);
  connect(this,&FactTree::structChanged,this,&FactTree::sizeChanged);
}
//=============================================================================
// fact or group
FactTree::FactTree(FactTree *parent, QString name, QString title, QString descr, ItemType type, QString alias)
 : QAbstractListModel(parent),m_parentItem(parent),itemType(type),
   m_level(0),
   m_name(name),m_title(title),m_descr(descr),m_alias(alias),
   m_enabled(true), m_visible(true)
{
  setObjectName(m_name);
  parent->addItem(this);

  //find tree item level
  for(FactTree *item=this;!item->isRoot();item=item->m_parentItem){
    m_level++;
  }

  emit structChanged(this);
  connect(this,&FactTree::structChanged,this,&FactTree::sizeChanged);
}
//=============================================================================
FactTree::~FactTree()
{
  /*if(m_parentItem){
    m_parentItem->removeItem(this);
  }*/
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
void FactTree::activate(void)
{
  //qDebug()<<"activate"<<name();
  emit triggered();
}
//=============================================================================
QVariant FactTree::value(void) const
{
  return m_value;
}
//=============================================================================
bool FactTree::setValue(const QVariant &v)
{
  QVariant vx=v;
  if(isFact() && size()>0){
    //list or enum
    FactTree *item=this;
    QString s=v.toString();
    bool ok=false;
    uint i=s.toUInt(&ok);
    if(ok && (int)i<size()) item=child(i);
    else item=childByName(v.toString());
    //qDebug()<<s<<item->name();
    if(item!=this)vx=item->num();
    else return false;
  }
  if(m_value==vx)return false;
  m_value=vx;
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
QString FactTree::title(void) const
{
  return m_title.isEmpty()?m_name:m_title;
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
int FactTree::size() const
{
  return m_items.size();
}
bool FactTree::enabled() const
{
  return m_enabled;
}
void FactTree::setEnabled(const bool &v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged(this);
}
bool FactTree::visible() const
{
  return m_visible;
}
void FactTree::setVisible(const bool &v)
{
  if(m_visible==v)return;
  m_visible=v;
  emit visibleChanged(this);
}
QString FactTree::section() const
{
  return m_section;
}
void FactTree::setSection(const QString &v)
{
  if(m_section==v)return;
  m_section=v;
  emit sectionChanged(this);
}
QString FactTree::text() const
{
  FactTree *item=valueEnumItem();
  if(item) return item->name();
  return value().toString();
}
void FactTree::setText(const QString &v)
{
  setValue(v);
}
//=============================================================================
bool FactTree::isFact(void) const
{
  return itemType==FactItem;
}
bool FactTree::isFactsList(void) const
{
  return itemType==GroupItem && (!m_items.isEmpty()) && m_items.first()->isFact();
}
bool FactTree::isRoot(void) const
{
  return itemType==RootItem;
}
bool FactTree::isBool(void) const
{
  if(!isFact())return false;
  return m_value.type()==QVariant::Bool;
}
bool FactTree::isText(void) const
{
  //if(!isFact())return false;
  return m_value.type()==QVariant::String;
}
//=============================================================================
FactTree *FactTree::parentItem() const
{
  return m_parentItem;
}
FactTree *FactTree::child(int n) const
{
  if(n>=m_items.size())return NULL;
  return m_items.at(n);
}
QList<FactTree*> FactTree::childItems() const
{
  return m_items;
}
//=============================================================================
const QChar FactTree::pathDelimiter='.';
//=============================================================================
FactTree * FactTree::childByName(const QString &itemName) const
{
  foreach(FactTree *item,m_items){
    if(item->name()==itemName)return item;
  }
  return const_cast<FactTree*>(this);
}
FactTree * FactTree::findByPath(const QString &itemNamePath) const
{
  QString s=itemNamePath;
  FactTree *item=const_cast<FactTree*>(this);
  while(!s.isEmpty()){
    if(item->isFactsList()){
      item=item->childByName(s);
      break;
    }else{
      int i=s.indexOf(pathDelimiter);
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
FactTree * FactTree::fact(const QString &factNamePath) const
{
  foreach(FactTree *i,m_items){
    if(i->isFact() && (i->name()==factNamePath || i->path().endsWith(factNamePath))) return i;
  }
  foreach(FactTree *i,m_items){
    i=i->fact(factNamePath);
    if(i)return i;
  }
  return NULL;
}
QList<FactTree *> FactTree::allFacts() const
{
  QList<FactTree*> list;
  foreach(FactTree *i,m_items){
    if(i->isFact()){
      list.append(static_cast<FactTree*>(i));
    }else{
      list.append(i->allFacts());
    }
  }
  return list;
}
QString FactTree::path(int fromLevel) const
{
  QString s=name();
  for(const FactTree *i=parentItem();i && i->level()>=fromLevel;i=i->parentItem()){
    s.prepend(i->name()+pathDelimiter);
    if(i->isRoot() && fromLevel>=0)break;
  }
  return s;
}
FactTree * FactTree::valueEnumItem() const
{
  if(isFact() && size()>0){
    //list or enum
    uint n=value().toUInt();
    if((int)n<size()){
      return child(n);
    }
  }
  return NULL;
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
  roles[ModelDataRole]   = "modelData";
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
  if(isFact()){
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
    case ModelDataRole:  return QVariant::fromValue(item);
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
