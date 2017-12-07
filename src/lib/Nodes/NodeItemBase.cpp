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
#include <Facts.h>
#include "NodeItemBase.h"
//=============================================================================
NodeItemBase::NodeItemBase(Fact *parent, const QString &name, const QString &title)
  : NodeFieldBase(parent,name,title,"",GroupItem,NoData)
{
}
QStringList NodeItemBase::sortNames=QStringList()
  <<"shiva"<<"nav"<<"cas"<<"gps"
  <<"ifc"<<"swc"<<"mhx"<<"servo"<<"bldc";
//=============================================================================
QVariant NodeItemBase::data(int col, int role) const
{
  if(dictValid() && dataValid()){
    switch(role){
      case Qt::ForegroundRole:
        if(col==FACT_MODEL_COLUMN_DESCR)
          return QColor(Qt::darkGray);
        if(col==FACT_MODEL_COLUMN_VALUE)
          return QColor(Qt::yellow).lighter(180);
      break;
      case Qt::BackgroundRole: return QColor(0x10,0x20,0x30);
    }
  }
  return NodeFieldBase::data(col,role);
}
//=============================================================================
bool NodeItemBase::lessThan(Fact *rightFact) const
{
  //try to sort by sortNames
  QString sleft=title().toLower();
  if(sleft.contains('.'))sleft=sleft.remove(0,sleft.indexOf('.')+1).trimmed();
  QString sright=rightFact->title().toLower();
  if(sright.contains('.'))sright=sright.remove(0,sright.indexOf('.')+1).trimmed();
  if(sortNames.contains(sleft)){
    if(sortNames.contains(sright)){
      int ileft=sortNames.indexOf(sleft);
      int iright=sortNames.indexOf(sright);
      if(ileft!=iright) return ileft<iright;
    }else return true;
  }else if(sortNames.contains(sright)) return false;

  //compare names
  int ncmp=QString::localeAwareCompare(title().toLower(),rightFact->title().toLower());
  if(ncmp!=0)return ncmp<0;
  //try to sort by comment same names
  ncmp=QString::localeAwareCompare(text().toLower(), rightFact->text().toLower());
  if(ncmp==0){
    //try to sort by sn same names
    const NodeItem *lnode=qobject_cast<const NodeItem*>(this);
    const NodeItem *rnode=qobject_cast<const NodeItem*>(rightFact);
    if(lnode && rnode) ncmp=QString::localeAwareCompare(QString(lnode->sn.toHex()), QString(rnode->sn.toHex()));
  }
  if(ncmp==0) return Fact::lessThan(rightFact);
  return ncmp<0;
}
//=============================================================================
