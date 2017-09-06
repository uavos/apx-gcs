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
#include "NodesItemNgrp.h"
#include "NodesItemField.h"
#include "NodesItemNode.h"
//=============================================================================
NodesItemNgrp::NodesItemNgrp(NodesItem *parent, QString name, QString descr)
 : NodesItem(parent,name,descr)
{
  item_type=it_ngroup;
}
//=============================================================================
QVariant NodesItemNgrp::data(int column,int role) const
{
  switch(role){
  case Qt::ForegroundRole:
    if(column==tc_field)return isModified()?QColor(Qt::red).lighter():QVariant();
    if(column==tc_value)return isValid()?QColor(Qt::darkGray):QVariant();
    return QColor(Qt::darkGray);
  case Qt::BackgroundRole:
    if(!isValid())return QColor(40,40,0);
    return QColor(0x10,0x30,0x30);//isModified()?QColor(0x40,0x20,0x20):
  case Qt::FontRole:
    return QFont("Monospace",-1,QFont::Bold);
  case Qt::ToolTipRole:
    QString s=name;
    foreach(NodesItem *i,childItems)
      s+=QString("\n%1: %2").arg(i->name).arg(i->getValue().toString());
    return s;
  }
  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(column){
    case tc_field: return name.toUpper();//QString("%1 (x%2)").arg(name).arg(childItems.size());
    case tc_value: return QString("[%1]").arg(childItems.size());//QVariant();
    case tc_descr: return descr;
  }
  return QVariant();
}
//=============================================================================
Qt::ItemFlags NodesItemNgrp::flags(int column) const
{
  Qt::ItemFlags f=NodesItem::flags(column);
  f|=Qt::ItemIsEnabled;
  if(column==tc_value && childItems.size())
    return f|Qt::ItemIsEditable;
  return f;
}
//=============================================================================
uint NodesItemNgrp::progress() const
{
  if(!childItems.size())return 0;
  int cnt=0;
  int cnt0=0;
  if(isUpgrading()){
    foreach(const NodesItem *i,childItems){
      const NodesItemNode *node=static_cast<const NodesItemNode*>(i);
      if(node->isUpgrading()){
        cnt+=node->progress();
        cnt0++;
      }else if(node->isUpgradePending())cnt0++;
    }
    if(cnt0)cnt/=cnt0;
    return cnt;
  }
  foreach(const NodesItem *i,childItems){
    uint ip=static_cast<const NodesItemNode*>(i)->progress();
    if(ip)cnt+=ip;
    else cnt0++;
  }
  if(cnt){
    cnt+=cnt0*100;
    cnt/=childItems.size();
  }
  return cnt;
}
//=============================================================================
