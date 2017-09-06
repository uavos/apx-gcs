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
#include "MissionItem.h"
#include "QMandala.h"
//=============================================================================
MissionItem::MissionItem(MissionItem *parent, QString name, QString caption, QString descr)
 : QObject(),parentItem(parent),m_caption(caption),m_descr(descr)
{
  setObjectName(name);
  if(caption.isEmpty())m_caption=name;
  if(parent)parent->appendChild(this);
}
//=============================================================================
QVariant MissionItem::data(int column,int role) const
{
  switch(role){
    case Qt::ForegroundRole:
      if(!parentItem->parent()) return QColor(Qt::darkGray);
      if(column==tc_field){
        if(isModified())return QColor(Qt::yellow).lighter();
        return QColor(Qt::white);
      }
      if(column==tc_value){
        if(isModified())return QColor(Qt::yellow).lighter();
        if(isZero())return QColor(Qt::gray);
        if(childCount()<=0) return QColor(220,240,240);
        return QColor(Qt::cyan).lighter(180);
      }
      //if(column>tc_value) return darkStyle?Qt::cyan:Qt::darkCyan;
      return QVariant();
    case Qt::BackgroundRole:
      return QVariant();
    case Qt::FontRole:
      //if(parentItem->inherits("MissionItemRw"))
      if(childCount()<=0)
        return QFont("Monospace",-1,QFont::Normal);
      return QFont("Monospace",-1,column==tc_field?QFont::Bold:QFont::Normal);
    case Qt::ToolTipRole:
      QStringList st=getToolTip();
      if(!st.size())return QVariant();
      else{
        //reformat name: value
        QString s="<html><NOBR><table>";
        s+="<tr><td colspan=2 align=left style='font-family: monospace; font-weight: bold;'><PRE><font size=+1>";
        s+=st.takeFirst();
        s+="</font></PRE></td></tr>";
        foreach(QString sl,st){
          if(sl.isEmpty())s+="<tr><td colspan=2><hr size=1></td></tr>";
          else if(sl.contains(':')) s+=QString("<tr><td align=right><b>%1</b>&nbsp;&nbsp;</td><td align=left>%2</td></tr>").arg(sl.left(sl.indexOf(':')+1)).arg(sl.mid(sl.indexOf(':')+1));
          else s+=QString("<tr><td colspan=2>%1</td></tr>").arg(sl);
        }
        s+="</table>";
        return s;
      }
  }

  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(column){
    case tc_field: return caption();
    case tc_value: return isZero()?"off":value();
  }
  return QVariant();
}
//=============================================================================
QVariant MissionItem::value(void) const
{
  if(!childCount()) return QVariant();
  if(isZero())return QVariant();//"off";
  //actions value for display
  QStringList st;
  foreach(MissionItem *i,childItems){
    if(!i->isZero())st.append(i->caption());
  }
  return st.join(',');
}
//=============================================================================
bool MissionItem::setValue(QVariant value)
{
  Q_UNUSED(value)
  return true;
}
//=============================================================================
QStringList MissionItem::getToolTip(void) const
{
  QStringList st;
  QString s=data(0).toString();
  QString sv=data(1).toString();//getValue().toString();
  if(sv.size()) s+=sv.startsWith('(')?(" "+sv):(" ("+sv+")");
  st.append(s);
  if(!descr().isEmpty())st.append(descr());
  foreach(MissionItem *i,childItems){
    if(!i->isZero())
      st.append(QString("%1: %2").arg(i->data(0).toString()).arg(i->data(1).toString()));
  }
  return st;
}
//=============================================================================
bool MissionItem::isModified(void) const
{
  foreach(MissionItem *i,childItems)
    if(i->isModified())return true;
  return false;
}
//=============================================================================
void MissionItem::clear(void)
{
  //qDebug()<<"clear"<<this;
  foreach(MissionItem *i,childItems){
    i->clear();
    i->deleteLater();
    //delete i;
  }
  //qDeleteAll(childItems);
  childItems.clear();
}
//=============================================================================
void MissionItem::remove(void)
{
  if(parentItem)parentItem->removeChild(this);
  clear();
  deleteLater();
}
//=============================================================================
void MissionItem::backup(void)
{
  foreach(MissionItem *i,childItems){
    i->backup();
  }
}
//=============================================================================
void MissionItem::invalidate(void)
{
  foreach(MissionItem *i,childItems){
    i->invalidate();
  }
}
//=============================================================================
void MissionItem::restore(void)
{
  foreach(MissionItem *i,childItems)
    i->restore();
  return;
}
//=============================================================================
bool MissionItem::isZero(void) const
{
  foreach(MissionItem *i,childItems)
    if(!i->isZero())return false;
  return true;
}
//=============================================================================
//=============================================================================
MissionItem * MissionItem::find(QString iname,MissionItem *scope)
{
  foreach(MissionItem *i,scope->childItems)
    if(i->objectName()==iname)
      return i;
  return NULL;
}
//=============================================================================
void MissionItem::appendChild(MissionItem *item)
{
  childItems.append(item);
  item->parentItem=this;
}
//=============================================================================
void MissionItem::removeChild(MissionItem *item)
{
  childItems.removeOne(item);
  item->parentItem=NULL;
}
//=============================================================================
MissionItem *MissionItem::child(int row)
{
  foreach(MissionItem *i,childItems)
    if(i->row()==row)return i;
  MissionItem *i=childItems.value(row);
  return i?i:this;
}
//=============================================================================
int MissionItem::childCount() const
{
  return childItems.size();
}
//=============================================================================
int MissionItem::columnCount() const
{
  return 2;
}
//=============================================================================
bool MissionItem::setData(int column, const QVariant & value)
{
  if(column!=tc_value)return false;
  if(setValue(value)){
    emit edited();
    return true;
  }
  return false;
}
//=============================================================================
MissionItem *MissionItem::parent()
{
  return parentItem;
}
//=============================================================================
int MissionItem::row() const
{
  if(!parentItem) return 0;
  return parentItem->childItems.indexOf(const_cast<MissionItem*>(this));
}
//=============================================================================
Qt::ItemFlags MissionItem::flags(int column) const
{
  Q_UNUSED(column)
  return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}
//=============================================================================
QString MissionItem::name() const
{
  return namePrefix+QString::number(row()+1);
}
QString MissionItem::caption() const
{
  return m_caption;
}
const QString & MissionItem::descr() const
{
  return m_descr;
}
void MissionItem::setDescr(const QString &v)
{
  m_descr=v;
  emit changed();
}
QString MissionItem::text() const
{
  return data(tc_value,Qt::DisplayRole).toString();
}
QQmlListProperty<MissionItem> MissionItem::items()
{
  return QQmlListProperty<MissionItem>(this,0,&itemsCount,&itemAt);
}
//=============================================================================
MissionItem *MissionItem::child(const QString &vname) const
{
  foreach(MissionItem *i,childItems)
    if(i->objectName()==vname) return i;
  return NULL;
}
//=============================================================================
QList<MissionItem*> MissionItem::childItemsFlat(void) const
{
  QList<MissionItem*> list;
  foreach(MissionItem *i,childItems){
    if(i->childCount())list.append(i->childItemsFlat());
    else list.append(i);
  }
  return list;
}
//=============================================================================
QByteArray MissionItem::md5() const
{
  QByteArray ba;
  if(!parentItem){ //root
    ba.append(QByteArray((const char*)&QMandala::instance()->current->home_pos[0],sizeof(QMandala::instance()->current->home_pos[0])));
    ba.append(QByteArray((const char*)&QMandala::instance()->current->home_pos[1],sizeof(QMandala::instance()->current->home_pos[1])));
    ba.append(QByteArray((const char*)&QMandala::instance()->current->home_pos[2],sizeof(QMandala::instance()->current->home_pos[2])));
  }
  foreach(MissionItem *i,childItems)
    ba.append(i->md5());
  return QCryptographicHash::hash(ba,QCryptographicHash::Md5);
}
//=============================================================================
void MissionItem::saveToXml(QDomNode dom) const
{
  foreach(MissionItem *i,childItems)
    i->saveToXml(dom);
}
//=============================================================================
void MissionItem::loadFromXml(QDomNode dom)
{
  foreach(MissionItem *i,childItems)
    i->loadFromXml(dom);
}
//=============================================================================
QByteArray MissionItem::pack() const
{
  QByteArray ba;
  foreach(MissionItem *i,childItems)
    ba.append(i->pack());
  return ba;
}
int MissionItem::unpack(const QByteArray &ba)
{
  int cnt=0;
  while(cnt<ba.size()){
    int scan_cnt=0;
    foreach(MissionItem *i,childItems){
      int rcnt=i->unpack(ba.mid(cnt));
      cnt+=rcnt;
      scan_cnt+=rcnt;
      if(cnt>=ba.size())break;
    }
    if(!scan_cnt)return 0; //error
  }
  return cnt;
}
//=============================================================================
