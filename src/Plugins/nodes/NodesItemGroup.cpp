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
#include "NodesItemGroup.h"
#include "NodesItemField.h"
//=============================================================================
NodesItemGroup::NodesItemGroup(NodesItem *parent, QString name, QString descr)
 : NodesItem(parent,name,descr)
{
  item_type=it_group;
}
//=============================================================================
QVariant NodesItemGroup::data(int column,int role) const
{
  switch(role){
  case Qt::ForegroundRole:
    if(column==tc_field){
      if(!isValid())return QColor(Qt::darkYellow);
      if(isModified())return QColor(Qt::red).lighter();
      if(isValueArray())return QColor(Qt::white);
      return QColor(Qt::gray);
    }
    if(column==tc_value){
      if(isValueArray())return QColor(Qt::cyan).lighter(180);
      return QColor(Qt::darkGray);
    }
    return QVariant();
  case Qt::BackgroundRole:
    return QVariant();
  case Qt::FontRole:
    if(column==tc_field){
      if(parentItem->item_type==it_group)
        return QFont("Monospace",-1,QFont::Normal);
      return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
    }
    return QVariant();
  case Qt::ToolTipRole:
    QString s=name;
    foreach(NodesItem *i,childItems)
      s+=QString("\n%1: %2").arg(i->name).arg(i->data(tc_value,Qt::DisplayRole).toString());
    return s;
  }
  if(column==tc_value && isValueArray() && (role==Qt::DisplayRole || role==Qt::EditRole)){
    NodesItemField *item=static_cast<NodesItemField*>(childItems.first());
    uint usedCnt=0;
    foreach(NodesItem *i,item->childItems){
      if(!i->isZero())usedCnt++;
    }
    return QVariant(usedCnt?QString("[%1/%2]").arg(usedCnt).arg(item->array):QString("[%1]").arg(item->array));
  }
  return NodesItem::data(column,role);
}
//=============================================================================
QVariant NodesItemGroup::getValue(void) const
{
  if(isZero())return "default";
  if(childItems.first()->item_type!=it_field)return QVariant();
  QString s;
  NodesItemField *f=static_cast<NodesItemField*>(childItems.first());
  if(f->ftype==ft_option || f->ftype==ft_string)s=f->getValue().toString();
  if(childItems.size()>1){
    NodesItemField *f=static_cast<NodesItemField*>(childItems.at(1));
    if(f->ftype==ft_string && (!f->isZero()))s=f->getValue().toString()+" ("+s+")";
  }
  return QVariant(s);
}
//=============================================================================
Qt::ItemFlags NodesItemGroup::flags(int column) const
{
  Qt::ItemFlags f=NodesItem::flags(column);
  if(column==tc_value && isValueArray()) f|=Qt::ItemIsEditable;
  return f;
}
//=============================================================================
bool NodesItemGroup::isValueArray(void) const
{
  if(childItems.size()<2)return false;
  bool bArray=false;
  uint cnt=0;
  foreach(NodesItem *i,childItems){
    bArray=false;
    if(i->item_type!=it_field)break;
    NodesItemField *field_item = static_cast<NodesItemField*>(i);
    if(cnt<2 && field_item->array<=1)break;
    if((int)field_item->array!=field_item->childCount())break;
    //if(!array_sz)array_sz=field_item->array;
    //if(field_item->array!=array_sz)break;
    cnt++;
    bArray=true;
  }
  return bArray;
}
//=============================================================================

