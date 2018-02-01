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
FactAction::FactAction(Fact *parent, const QString &name, const QString &title, const QString &descr, FactActionType actionType, const QString &icon)
 : QObject(parent),
   fact(parent),
   m_name(name),
   m_title(title),
   m_descr(descr),
   m_icon(icon),
   m_actionType(actionType),
   m_enabled(true),
   m_visible(true)
{
  setObjectName(m_name);
  parent->actions.append(this);
}
//=============================================================================
void FactAction::trigger(void)
{
  if(!enabled())return;
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
FactAction::FactActionType FactAction::actionType(void) const
{
  return m_actionType;
}
bool FactAction::enabled() const
{
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
