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
FactTree::FactTree(FactTree *parent, ItemType treeItemType)
 : QAbstractListModel(parent),m_parentItem(parent),m_treeItemType(treeItemType),
   m_level(0)
{
  if(parent)parent->addItem(this);

  //find tree item level
  for(FactTree *item=this;item->treeItemType()!=RootItem;item=item->m_parentItem){
    m_level++;
  }
  connect(this,&FactTree::structChanged,this,&FactTree::sizeChanged);
}
//=============================================================================
void FactTree::addItem(FactTree *item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  m_items << item;
  item->m_parentItem=this;
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
  item->deleteLater();
  emit structChanged(this);
}
//=============================================================================
int FactTree::num() const
{
  if(!m_parentItem) return 0;
  return m_parentItem->m_items.indexOf(const_cast<FactTree*>(this));
}
FactTree *FactTree::child(int n) const
{
  if(n>=m_items.size())return NULL;
  return m_items.at(n);
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
void FactTree::clear(void)
{
  foreach(FactTree *i,m_items){
    i->clear();
  }
  beginRemoveRows(QModelIndex(), 0, m_items.size());
  foreach (FactTree *item, m_items) {
    item->m_parentItem=NULL;
  }
  qDeleteAll(m_items);
  m_items.clear();
  endRemoveRows();
  emit structChanged(this);
}
//=============================================================================
//=============================================================================
FactTree::ItemType FactTree::treeItemType(void) const
{
  return m_treeItemType;
}
int FactTree::level(void) const
{
  return m_level;
}
int FactTree::size(void) const
{
  return m_items.size();
}
//=============================================================================
// LIST MODEL
//=============================================================================
int FactTree::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent)
  return size();
}
//=============================================================================
