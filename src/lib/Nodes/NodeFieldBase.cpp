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
#include "NodeFieldBase.h"
#include "Nodes.h"
//=============================================================================
NodeFieldBase::NodeFieldBase(Fact *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType)
  : Fact(parent,name,title,descr,treeItemType,dataType),
    m_dictValid(false),
    m_dataValid(false)
{
  connect(this,&NodeFieldBase::dataValidChanged,this,[=](){setEnabled(dataValid());});
  setEnabled(dataValid());

  connect(this,&Fact::parentItemChanged,this,&NodeFieldBase::addActions);
  addActions();
}
//=============================================================================
void NodeFieldBase::addActions()
{
  if(!actions.isEmpty())return;
  Nodes *nodes=parent_cast<Nodes*>();
  if(!nodes)return;

  FactAction *a=new FactAction(this,"revert",tr("Revert"),tr("Undo changes"),"undo");
  connect(a,&FactAction::triggered,this,&Fact::restore);
  connect(this,&Fact::modifiedChanged,a,[=](){a->setEnabled(modified());});
  a->setEnabled(modified());

  new FactAction(this,nodes->f_upload);
  foreach (FactAction *a, actions) {
    connect(a,&FactAction::enabledChanged,a,[=](){a->setVisible(a->enabled());});
    a->setVisible(a->enabled());
  }
}
//=============================================================================
QVariant NodeFieldBase::data(int col, int role) const
{
  switch(role){
    case Qt::ForegroundRole:
      if(!dictValid())
        return QColor(Qt::darkGray);
      if(!dataValid())
        return col==FACT_MODEL_COLUMN_NAME?QColor(Qt::darkYellow):QColor(Qt::darkGray);
    break;
    case Qt::BackgroundRole:
      if(!(dictValid() && dataValid()))
        return QVariant();
    break;
  }
  return Fact::data(col,role);
}
//=============================================================================
//=============================================================================
bool NodeFieldBase::dictValid() const
{
  return m_dictValid;
}
void NodeFieldBase::setDictValid(const bool &v, bool recursive)
{
  if(recursive){
    foreach (FactTree *i, childItems()) {
      static_cast<NodeFieldBase*>(i)->setDictValid(v);
    }
  }
  if(m_dictValid==v)return;
  m_dictValid=v;
  emit dictValidChanged();
}
bool NodeFieldBase::dataValid() const
{
  return m_dataValid;
}
void NodeFieldBase::setDataValid(const bool &v,bool recursive)
{
  if(v && (!dictValid()))return;
  if(recursive){
    foreach (FactTree *i, childItems()) {
      static_cast<NodeFieldBase*>(i)->setDataValid(v);
    }
  }
  if(m_dataValid==v)return;
  m_dataValid=v;
  emit dataValidChanged();
  emit enabledChanged();//to update model view
  //qDebug()<<"dataValid"<<v<<path();
}
//=============================================================================
//=============================================================================
