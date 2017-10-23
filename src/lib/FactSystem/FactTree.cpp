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
 : QAbstractListModel(parent),
   _bindedChildsFact(NULL),
   m_parentItem(parent),
   m_treeItemType(treeItemType),
   m_level(0),m_flatModel(false)
{
  if(parent)parent->addItem(this);

  //find tree item level
  for(FactTree *item=this;item->treeItemType()!=RootItem;item=item->m_parentItem){
    m_level++;
  }
  connect(this,&FactTree::structChanged,this,&FactTree::sizeChanged);
  if(parent && parent->flatModel()){
    //connect(this,&FactTree::structChanged,parent,&FactTree::structChanged);
  }
}
//=============================================================================
void FactTree::bindChilds(FactTree *item)
{
  _bindedChildsFact=item;
  connect(item,&FactTree::structChanged,this,&FactTree::structChanged);
}
//=============================================================================
void FactTree::removeItem(FactTree *item)
{
  if(_bindedChildsFact){
    _bindedChildsFact->removeItem(item);
    return;
  }
  int i=m_items.indexOf(item);
  if(i<0)return;
  bool bFlat=m_parentItem->flatModel();
  if(bFlat){
    int i2=m_parentItem->childItems().indexOf(item);
    m_parentItem->beginRemoveRows(QModelIndex(), i2 ,i2);
  }
  beginRemoveRows(QModelIndex(), i, i);
  m_items.removeOne(item);
  item->m_parentItem=NULL;
  endRemoveRows();
  if(bFlat) m_parentItem->endRemoveRows();
  item->deleteLater();
  emit structChanged(this);
}
void FactTree::insertItem(int i, FactTree *item)
{
  if(_bindedChildsFact){
    _bindedChildsFact->insertItem(i,item);
    return;
  }
  beginInsertRows(QModelIndex(), i, i);
  m_items.insert(i,item);
  bool bFlat=m_parentItem && m_parentItem->flatModel();
  if(bFlat){
    int i2=m_parentItem->childItems().indexOf(item);
    m_parentItem->beginInsertRows(QModelIndex(), i2 ,i2);
  }
  item->m_parentItem=this;
  endInsertRows();
  if(bFlat) m_parentItem->endInsertRows();
  emit structChanged(this);
}
void FactTree::addItem(FactTree *item)
{
  insertItem(m_items.size(),item);
}
void FactTree::moveItem(FactTree *item,int dest)
{
  moveRows(QModelIndex(),item->num(),1,QModelIndex(),dest);
}
//=============================================================================
int FactTree::num() const
{
  if(!m_parentItem) return 0;
  return m_parentItem->m_items.indexOf(const_cast<FactTree*>(this));
}
FactTree *FactTree::child(int n) const
{
  QList<FactTree*> list=childItems();
  if(n>=list.size())return NULL;
  return list.at(n);
}
FactTree *FactTree::parentItem() const
{
  return m_parentItem;
}
QList<FactTree*> FactTree::childItems() const
{
  if(m_flatModel){
    QList<FactTree*> list;
    foreach(FactTree *item,m_items){
      if(item->treeItemType()==SectionItem) {
        list.append(item->childItems());
      }else list.append(item);
    }
    return list;
  }
  if(_bindedChildsFact)return _bindedChildsFact->childItems();
  return m_items;
}
//=============================================================================
void FactTree::clear(void)
{
  if(_bindedChildsFact) return;

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
  if(m_flatModel){
    int sz=0;
    foreach(const FactTree*i,m_items){
      if(i->treeItemType()==SectionItem) sz+=i->size();
      else sz++;
    }
    return sz;
  }
  if(_bindedChildsFact)return _bindedChildsFact->size();
  return m_items.size();
}
bool FactTree::flatModel() const
{
  return m_flatModel;
}
void FactTree::setFlatModel(const bool &v)
{
  if(m_flatModel==v)return;
  m_flatModel=v;
  if(v){
    foreach(const FactTree*i,m_items){
      if(i->treeItemType()==SectionItem){
        //connect(i,&FactTree::structChanged,this,&FactTree::structChanged);
      }
    }
  }else{
    foreach(const FactTree*i,m_items){
      if(i->treeItemType()==SectionItem){
        //disconnect(i,&FactTree::structChanged,this,&FactTree::structChanged);
      }
    }
  }
  emit flatModelChanged();
  emit structChanged(this);
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
bool FactTree::moveRows(const QModelIndex &sourceParent, int src, int cnt, const QModelIndex &destinationParent, int dst)
{
  if(m_flatModel)return false;
  if(_bindedChildsFact)return _bindedChildsFact->moveRows(sourceParent,src,cnt,destinationParent,dst);
  if((src+cnt)>m_items.size())return false;
  if(dst>m_items.size())return false;
  if(src==dst)return true;
  beginMoveRows(sourceParent,src,src+cnt-1,destinationParent,dst);
  if(dst < src) for(int i  = 0; i < cnt; ++i) {
    m_items.insert(dst+i, m_items.takeAt(src+i));
  }else for(int i  = 0; i < cnt; ++i) {
    m_items.insert(dst-1, m_items.takeAt(src));
  }
  endMoveRows();
  emit structChanged(this);
  return true;
}
//=============================================================================
//=============================================================================
