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
#include "Fact.h"
#include "FactSystem.h"
//=============================================================================
Fact::Fact(FactTree *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType)
 : FactData(parent,name,title,descr,treeItemType,dataType),
   m_enabled(true), m_visible(true), m_active(false)
{
  if((treeItemType==GroupItem || treeItemType==SectionItem) && m_dataType==ConstData){
    connect(this,&Fact::sizeChanged,this,&Fact::statusChanged);
  }
}
//=============================================================================
void Fact::bind(FactData *item)
{
  if(_bindedFact){
    disconnect(static_cast<Fact*>(_bindedFact),&Fact::statusChanged,this,&Fact::statusChanged);
  }
  if(item==this)return;
  FactData::bind(item);
  connect(static_cast<Fact*>(item),&Fact::statusChanged,this,&Fact::statusChanged);
  emit statusChanged();
}
//=============================================================================
QVariant Fact::findValue(const QString &namePath)
{
  Fact *f=fact(namePath);
  if(!f){
    qWarning("FactSystem fact not found: %s",namePath.toUtf8().data());
    return QVariant();
  }
  if(f->dataType()==Fact::EnumData)return f->text();
  return f->value();
}
//=============================================================================
Fact * Fact::fact(const QString &factNamePath) const
{
  foreach(FactTree *item,childItems()){
    Fact *f=static_cast<Fact*>(item);
    if(f->treeItemType()==FactItem && (f->name()==factNamePath || f->path().endsWith(factNamePath)))
      return f;
  }
  foreach(FactTree *item,childItems()){
    Fact *f=static_cast<Fact*>(item);
    f=f->fact(factNamePath);
    if(f)return f;
  }
  return NULL;
}
//=============================================================================
/*Fact * Fact::byPath(const QString &itemNamePath) const
{
  QString s=itemNamePath;
  Fact *item=const_cast<Fact*>(this);
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
}*/
//=============================================================================
void Fact::trigger(void)
{
  //qDebug()<<"trigger"<<name();
  emit triggered();
}
//=============================================================================
bool Fact::enabled() const
{
  return m_enabled;
}
void Fact::setEnabled(const bool &v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged();
}
bool Fact::visible() const
{
  return m_visible;
}
void Fact::setVisible(const bool &v)
{
  if(m_visible==v)return;
  m_visible=v;
  emit visibleChanged();
}
QString Fact::section() const
{
  return m_section;
}
void Fact::setSection(const QString &v)
{
  if(m_section==v)return;
  m_section=v;
  emit sectionChanged();
}
QString Fact::status() const
{
  if(_bindedFact) return static_cast<Fact*>(_bindedFact)->status();
  if((treeItemType()==GroupItem||treeItemType()==SectionItem) && m_dataType==ConstData && m_status.isEmpty()){
    return size()>0?QString::number(size()):QString();
  }
  return m_status;
}
void Fact::setStatus(const QString &v)
{
  if(_bindedFact){
    static_cast<Fact*>(_bindedFact)->setStatus(v);
    return;
  }
  if(m_status==v)return;
  m_status=v;
  emit statusChanged();
}
bool Fact::active() const
{
  if(_bindedFact) return static_cast<Fact*>(_bindedFact)->active();
  return m_active;
}
void Fact::setActive(const bool &v)
{
  if(_bindedFact){
    static_cast<Fact*>(_bindedFact)->setActive(v);
    return;
  }
  if(m_active==v)return;
  m_active=v;
  emit activeChanged();
}
int Fact::progress() const
{
  if(_bindedFact) return static_cast<Fact*>(_bindedFact)->progress();
  return m_progress;
}
void Fact::setProgress(const int &v)
{
  if(_bindedFact){
    static_cast<Fact*>(_bindedFact)->setProgress(v);
    return;
  }
  if(m_progress==v)return;
  m_progress=v;
  emit progressChanged();
}
QString Fact::qmlMenu() const
{
  return m_qmlMenu;
}
void Fact::setQmlMenu(const QString &v)
{
  if(m_qmlMenu==v)return;
  m_qmlMenu=v;
  emit qmlMenuChanged();
}
//=============================================================================
//=============================================================================
