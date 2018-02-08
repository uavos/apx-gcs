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
#include "FactAction.h"
#include "Fact.h"
#include <QColor>
#include <QFont>
#include <QApplication>
//=============================================================================
FactAction::FactAction(Fact *parent, const QString &name, const QString &title, const QString &descr, const QString &icon, uint flags)
 : QObject(parent),
   linkAction(NULL),
   m_name(name),
   m_title(title),
   m_descr(descr),
   m_icon(icon),
   m_flags(flags),
   m_enabled(true),
   m_visible(true),
   m_fact(NULL)
{
  setObjectName(m_name);
  parent->actions.append(this);
  connect(this,&FactAction::triggered,parent,&Fact::actionTriggered);
  //default icons
  if(m_icon.isEmpty()){
    if(flags&ActionApply)m_icon="check";
    else if(flags&ActionRemove)m_icon="delete";
  }
}
FactAction::FactAction(Fact *parent, FactAction *linkAction)
 : QObject(parent),
   linkAction(linkAction),
   m_name(linkAction->name()),
   m_title(linkAction->title()),
   m_descr(linkAction->descr()),
   m_icon(linkAction->icon()),
   m_flags(linkAction->flags()),
   m_enabled(true),
   m_visible(linkAction->visible()),
   m_fact(NULL)
{
  setObjectName(m_name);
  parent->actions.append(this);
  connect(this,&FactAction::triggered,parent,&Fact::actionTriggered);

  connect(linkAction,&FactAction::enabledChanged,this,&FactAction::enabledChanged);
}
FactAction::FactAction(Fact *parent, Fact *pageFact)
 : QObject(parent),
   linkAction(NULL),
   m_name(pageFact->name()),
   m_title(pageFact->title()),
   m_descr(pageFact->descr()),
   m_icon(pageFact->icon()),
   m_flags(ActionPage),
   m_enabled(true),
   m_visible(true),
   m_fact(pageFact)
{
  setObjectName(m_name);
  parent->actions.append(this);
}
//=============================================================================
void FactAction::trigger(void)
{
  if(!enabled())return;
  if(linkAction)linkAction->trigger();
  //qDebug()<<"trigger"<<name();
  emit triggered();
}
//=============================================================================
QString FactAction::name(void) const
{
  return m_name;
}
void FactAction::setName(const QString &v)
{
  if(m_name==v)return;
  m_name=v;
  setObjectName(v);
  emit nameChanged();
}
QString FactAction::title(void) const
{
  return m_title;
}
void FactAction::setTitle(const QString &v)
{
  QString s=v.trimmed();
  if(m_title==s)return;
  m_title=s;
  emit titleChanged();
}
QString FactAction::descr(void) const
{
  return m_descr;
}
void FactAction::setDescr(const QString &v)
{
  QString s=v.trimmed();
  if(m_descr==s)return;
  m_descr=s;
  emit descrChanged();
}
QString FactAction::icon(void) const
{
  return m_icon;
}
void FactAction::setIcon(const QString &v)
{
  if(m_icon==v)return;
  m_icon=v;
  emit iconChanged();
}
int FactAction::flags(void) const
{
  return m_flags;
}
bool FactAction::enabled() const
{
  if(linkAction)return m_enabled && linkAction->enabled();
  return m_enabled;
}
void FactAction::setEnabled(const bool &v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  emit enabledChanged();
}
bool FactAction::visible() const
{
  return m_visible;
}
void FactAction::setVisible(const bool &v)
{
  if(m_visible==v)return;
  m_visible=v;
  emit visibleChanged();
}
Fact * FactAction::fact() const
{
  return m_fact;
}
void FactAction::setFact(Fact *v)
{
  if(m_fact==v)return;
  m_fact=v;
  emit factChanged();
}
//=============================================================================
QVariant FactAction::data(int col, int role) const
{
  switch(role){
    case Fact::ModelDataRole: return QVariant::fromValue(const_cast<FactAction*>(this));
    case Qt::ForegroundRole:
      if(col==Fact::FACT_MODEL_COLUMN_NAME){
        if(!enabled())return QColor(Qt::gray);
        return QColor(Qt::white);
      }
      if(col==Fact::FACT_MODEL_COLUMN_VALUE){
        if(!enabled())return QColor(Qt::darkGray);
        return QColor(Qt::blue).lighter(170);
      }
      return QColor(Qt::darkCyan);
    case Qt::BackgroundRole:
      return QVariant();
    case Qt::FontRole: {
      return QVariant();
    }
    case Qt::ToolTipRole:
      return descr();
  }

  //value roles
  if(role!=Qt::DisplayRole && role!=Qt::EditRole)
    return QVariant();

  switch(col){
    case Fact::FACT_MODEL_COLUMN_NAME: return title();
    case Fact::FACT_MODEL_COLUMN_VALUE: return QString("<exec>");
    case Fact::FACT_MODEL_COLUMN_DESCR: return descr();
  }
  return QVariant();
}
//=============================================================================
