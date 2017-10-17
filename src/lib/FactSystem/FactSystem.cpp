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
#include <QtQml>
#include <QQmlEngine>

#include "FactSystem.h"
#define VSTR_IMPL(a) #a
#define VSTR(a) VSTR_IMPL(a)
//=============================================================================
FactSystem::FactSystem(QObject *parent)
 : QObject(parent)
{
  _instance=this;

  setObjectName("app");

  _tree=new Fact(NULL,objectName(),tr("System tree"),QCoreApplication::applicationName(),Fact::RootItem,Fact::NoData);


  // app constants and global facts
  Fact *item;

  item=new Fact(_tree,"version",tr("Version"),tr("Application version"),Fact::FactItem,Fact::TextData);
  QString s=VSTR(VERSION);
  if(s.isEmpty())s=tr("unknown");
  item->setValue(s);
  item->setEnabled(false);

  item=new Fact(_tree,"branch",tr("Branch"),tr("Source code repository branch"),Fact::FactItem,Fact::TextData);
  item->setValue(VSTR(BRANCH));
  item->setEnabled(false);

  item=new Fact(_tree,"dev",tr("Development mode"),"",Fact::FactItem,Fact::NoData);
  item->setEnabled(false);
#ifdef __ANDROID__
  item->setValue(false);
#else
  item->setValue(QCoreApplication::applicationDirPath().startsWith(QDir::homePath()));
#endif
  item->setVisible(item->value().toBool());

  item=new Fact(_tree,"test",tr("Test visual elements"),"",Fact::FactItem,Fact::BoolData);
  item->setValue(false);
  item->setSection(FactSystem::ApplicationSection);


  // QML types register
  qmlRegisterUncreatableType<Fact>("GCS.FactSystem", 1, 0, "Fact", "Reference only");
}
FactSystem::~FactSystem()
{
  _tree->deleteLater();
}
FactSystem * FactSystem::_instance=NULL;
//=============================================================================
const QString FactSystem::ApplicationSection=tr("Application");
const QString FactSystem::MandalaSection=tr("Application");
//=============================================================================
//=============================================================================
Fact * FactSystem::tree()
{
  return _instance->_tree;
}
//=============================================================================
//=============================================================================
void FactSystem::appendTree(Fact *tree)
{
  FactSystem::tree()->addItem(tree);
}
//=============================================================================
QJSValue FactSystem::_syncJS(QQmlEngine *e, Fact *factItem, QJSValue parent) //recursive
{
  QQmlEngine::setObjectOwnership(factItem,QQmlEngine::CppOwnership);
  QJSValue js_factItem=e->newQObject(factItem);
  parent.setProperty(factItem->name(),js_factItem);
  foreach(FactTree *i,factItem->childItems())
    _syncJS(e,static_cast<Fact*>(i),js_factItem);
  return js_factItem;
}
//=============================================================================
void FactSystem::syncJS(QQmlEngine *e)
{
  _instance->_syncJS(e,FactSystem::tree(),e->globalObject());
}
//=============================================================================
//=============================================================================
