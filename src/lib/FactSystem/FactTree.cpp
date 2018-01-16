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
FactTree::FactTree(FactTree *parent, const QString &name, ItemType treeItemType)
 : QObject(parent),
   m_parentItem(NULL),
   m_treeItemType(treeItemType),
   m_name(makeNameUnique(name)),
   m_num(0)
{
  setObjectName(m_name);
}
FactTree::~FactTree()
{
  //qDebug()<<"~FactTree";
  removeAll();
}
//=============================================================================
void FactTree::insertItem(int i, FactTree *item)
{
  if(i<0)i=0;
  item->m_parentItem=this;
  item->setParent(this);
  emit itemToBeInserted(i,item);
  m_items.insert(i,item);
  item->updateNum();
  emit itemInserted(item);
  emit sizeChanged();
}
void FactTree::removeItem(FactTree *item, bool deleteLater)
{
  int i=m_items.indexOf(item);
  if(i<0)return;
  item->removed();
  emit itemToBeRemoved(i,item);
  m_items.removeAt(i);
  for(int i=0;i<m_items.size();++i) m_items.at(i)->updateNum();
  if(deleteLater){
    //item->disconnect();
    item->deleteLater();
  }
  emit itemRemoved(item);
  emit sizeChanged();
}
void FactTree::removeAll(void)
{
  if(!m_items.size())return;
  for(int i=0;i<m_items.size();i++){
    m_items.at(i)->removeAll();
  }
  while(m_items.size()) {
    FactTree *item=m_items.last();
    item->removed();
    emit itemToBeRemoved(m_items.size()-1,item);
    m_items.takeLast();
    item->m_parentItem=NULL;
    emit itemRemoved(item);
    item->deleteLater();
  }
  m_items.clear();
  emit sizeChanged();
}
//=============================================================================
//=============================================================================
void FactTree::addItem(FactTree *item)
{
  insertItem(m_items.size(),item);
}
void FactTree::remove()
{
  if(m_parentItem)m_parentItem->removeItem(this);
}
//=============================================================================
QString FactTree::makeNameUnique(const QString &s)
{
  QString sr=QString(s)
      .replace(' ','_')
      .replace('.','_')
      .replace('-','_')
      .replace('+','_');
  if(!m_parentItem) return sr;
  int i=0;
  nameSuffix=QString();
  QString suffix;
  while(1){
    FactTree *dup=NULL;
    foreach(FactTree *item,m_parentItem->childItems()){
      if(item==this)continue;
      if(item->name()==(sr+suffix)){
        dup=item;
        break;
      }
    }
    if(!dup)break;
    suffix=QString("_%1").arg(++i,3,10,QChar('0'));
  }
  nameSuffix=suffix;
  return sr;
}
//=============================================================================
int FactTree::num() const
{
  return m_num;
}
void FactTree::updateNum()
{
  int v=0;
  if(m_parentItem) v=m_parentItem->m_items.indexOf(this);
  if(m_num==v)return;
  m_num=v;
  emit numChanged();
}
FactTree *FactTree::child(int n) const
{
  return childItems().value(n,NULL);
}
FactTree *FactTree::parentItem() const
{
  return m_parentItem;
}
QList<FactTree*> FactTree::childItems() const
{
  return m_items;
}
//=============================================================================
FactTree * FactTree::child(const QString &name) const
{
  foreach(FactTree *item,m_items){
    if(item->name()==name)return item;
  }
  return NULL;
}
//=============================================================================
QString FactTree::path(const QChar pathDelimiter) const
{
  QString s;
  for(const FactTree *i=this;i;i=i->parentItem()){
    if(s.isEmpty())s=i->name();
    else s.prepend(i->name()+pathDelimiter);
    if(i->treeItemType()==RootItem)break;
  }
  return s.isEmpty()?name():s;
}
QList<FactTree*> FactTree::pathList() const
{
  QList<FactTree*> list;
  for(const FactTree *i=this;i;i=i->parentItem()){
    list.append(const_cast<FactTree*>(i));
    if(i->treeItemType()==RootItem)break;
  }
  return list;
}
//=============================================================================
//=============================================================================
FactTree::ItemType FactTree::treeItemType(void) const
{
  return m_treeItemType;
}
void FactTree::setTreeItemType(const FactTree::ItemType &v)
{
  if(m_treeItemType==v)return;
  m_treeItemType=v;
  emit treeItemTypeChanged();
}
int FactTree::size(void) const
{
  return m_items.size();
}
QString FactTree::name(void) const
{
  return (m_name.contains('#')?QString(m_name).replace('#',QString::number(num()+1)):m_name)+nameSuffix;
}
void FactTree::setName(const QString &v)
{
  QString s=makeNameUnique(v);
  if(m_name==s && nameSuffix.isEmpty())return;
  //emit itemRemoved(this);
  m_name=s;
  setObjectName(name());
  //emit itemAdded(this);
  emit nameChanged();
}
//=============================================================================
