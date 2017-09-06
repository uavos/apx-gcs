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
#include "NodesItem.h"
//=============================================================================
NodesItem::NodesItem(NodesItem *parent, QString name, QString descr)
 : QObject(parent),parentItem(parent),name(name),descr(descr)
{
  item_type=it_root;
  if(parent)parent->appendChild(this);
}
//=============================================================================
QVariant NodesItem::data(int column,int role) const
{
  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(column){
    case tc_field: return name;
    case tc_value: return getValue();
    case tc_descr: return descr;
  }
  return QVariant();
}
//=============================================================================
QVariant NodesItem::getValue(void) const
{
  return QVariant();
}
//=============================================================================
bool NodesItem::setValue(const QVariant &value)
{
  Q_UNUSED(value)
  return true;
}
//=============================================================================
int NodesItem::getConfSize(void) const
{
  int sz=0;
  foreach(NodesItem *i,childItems)
    sz+=i->getConfSize();
  return sz;
}
//=============================================================================
bool NodesItem::isModified(void) const
{
  foreach(NodesItem *i,childItems)
    if(i->isModified())return true;
  return false;
}
//=============================================================================
bool NodesItem::isValid(void) const
{
  foreach(NodesItem *i,childItems)
    if(!i->isValid())return false;
  return true;
}
//=============================================================================
void NodesItem::clear(void)
{
  foreach(NodesItem *i,childItems) i->clear();
  qDeleteAll(childItems);
  childItems.clear();
}
//=============================================================================
void NodesItem::stop(void)
{
  foreach(NodesItem *i,childItems)
    i->stop();
}
//=============================================================================
void NodesItem::invalidate(void)
{
  foreach(NodesItem *i,childItems)
    i->invalidate();
}
//=============================================================================
void NodesItem::restore(void)
{
  foreach(NodesItem *i,childItems)
    i->restore();
}
//=============================================================================
bool NodesItem::isZero(void) const
{
  foreach(const NodesItem *i,childItems)
    if(!i->isZero())return false;
  return true;
}
//=============================================================================
bool NodesItem::isReconf(void) const
{
  foreach(const NodesItem *i,childItems)
    if(i->isReconf())return true;
  return false;
}
//=============================================================================
bool NodesItem::isUpgrading(void) const
{
  foreach(const NodesItem *i,childItems)
    if(i->isUpgrading())return true;
  return false;
}
bool NodesItem::isUpgradable(void) const
{
  foreach(const NodesItem *i,childItems)
    if(i->isUpgradable())return true;
  return false;
}
//=============================================================================
uint NodesItem::size(void) const
{
  uint cnt=1;
  foreach(const NodesItem *i,childItems)
    cnt+=i->size();
  return cnt;
}
uint NodesItem::sizePending(void) const
{
  if(isValid()&&(!isModified()))return 0;
  uint cnt=1;
  foreach(const NodesItem *i,childItems)
    cnt+=i->sizePending();
  return cnt;
}
//=============================================================================
//=============================================================================
NodesItem * NodesItem::find(QString iname,NodesItem *scope)
{
  foreach(NodesItem *i,scope->childItems)
    if(i->name==iname)
      return i;
  return NULL;
}
//=============================================================================
void NodesItem::appendChild(NodesItem *item)
{
  childItems.append(item);
  item->parentItem=this;
}
//=============================================================================
void NodesItem::removeChild(NodesItem *item)
{
  childItems.removeOne(item);
  item->parentItem=NULL;
}
//=============================================================================
NodesItem *NodesItem::child(int row)
{
  if(row>=childItems.size())return NULL;
  return childItems.at(row);
  /*foreach(NodesItem *i,childItems)
    if(i->row()==row)return i;
  //NodesItem *i=row<childItems.size()?childItems.at(row):NULL;
  return NULL;*/
}
//=============================================================================
int NodesItem::childCount() const
{
  return childItems.count();
}
//=============================================================================
int NodesItem::columnCount() const
{
  return 3;
}
//=============================================================================
bool NodesItem::setData(int column, const QVariant & value)
{
  if(column!=tc_value)return false;
  return setValue(value);
}
//=============================================================================
NodesItem *NodesItem::parent()
{
  return parentItem;
}
//=============================================================================
int NodesItem::row() const
{
  if(!parentItem) return 0;
  return parentItem->childItems.indexOf(const_cast<NodesItem*>(this));
}
//=============================================================================
Qt::ItemFlags NodesItem::flags(int column) const
{
  Q_UNUSED(column)
  if(isValid() && (!isUpgrading()))return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
  return Qt::ItemIsSelectable;
}
//=============================================================================
void NodesItem::sync(void)
{
  foreach(NodesItem *i,childItems)
    i->sync();
}
//=============================================================================
void NodesItem::stats(void)
{
  foreach(NodesItem *i,childItems)
    i->stats();
}
//=============================================================================
void NodesItem::upload(void)
{
  foreach(NodesItem *i,childItems)
    i->upload();
}
//=============================================================================
void NodesItem::saveToXml(QDomNode dom) const
{
  foreach(NodesItem *i,childItems)
    i->saveToXml(dom);
}
//=============================================================================
void NodesItem::loadFromXml(QDomNode dom)
{
  QCoreApplication::processEvents();
  foreach(NodesItem *i,childItems)
    i->loadFromXml(dom);
}
//=============================================================================
