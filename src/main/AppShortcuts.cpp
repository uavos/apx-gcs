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
#include <QQmlEngine>
#include "AppShortcuts.h"
#include "QMandala.h"
//=============================================================================
AppShortcuts::AppShortcuts(QWidget *parent)
  :QObject(parent), widget(parent), m_blocked(false)
{
  qmlRegisterType<AppShortcuts>();
  qmlRegisterType<AppShortcut>();
  qmlRegisterType<AppShortcutModel>();

  saveTimer.setSingleShot(true);
  saveTimer.setInterval(500);
  connect(&saveTimer,SIGNAL(timeout()),this,SLOT(save()));

  load();
  qApp->setProperty("shortcuts",qVariantFromValue(this));
}
//=============================================================================
void AppShortcuts::load()
{
  QSettings settingsSystem(QMandala::Global::res().filePath("templates/shortcuts.conf"),QSettings::IniFormat);
  foreach(QString key,settingsSystem.childKeys()){
    if(key.trimmed().startsWith("#"))continue;
    if(key.trimmed().isEmpty())continue;
    QString cmd;
    if(!key.contains("shortcut")){
      cmd=settingsSystem.value(key).toString();
    }else{
      QStringList st=settingsSystem.value(key).toStringList();
      if(st.size()!=2)continue;
      key=st.at(0);
      cmd=st.at(1);
    }
    AppShortcut *sc=new AppShortcut(this,widget);
    sc->setKey(key);
    sc->setCmd(cmd);
    m_systemShortcuts.addItem(sc);
  }
  m_newItem=new AppShortcut(this,widget);
  //user
  QSettings settingsUser;
  settingsUser.beginGroup("shortcuts");
  int size=settingsUser.beginReadArray("shortcut");
  for (int i = 0; i < size; ++i) {
    settingsUser.setArrayIndex(i);
    AppShortcut *sc=new AppShortcut(this,widget);
    sc->setEnabled(settingsUser.value("enabled").toBool());
    sc->setKey(settingsUser.value("key").toString());
    sc->setCmd(settingsUser.value("cmd").toString());
    m_userShortcuts.addItem(sc);
  }
  m_newItem=new AppShortcut(this,widget);
  connect(&m_userShortcuts,SIGNAL(changed()),&saveTimer,SLOT(start()));
  connect(&m_systemShortcuts,SIGNAL(changed()),&saveTimer,SLOT(start()));
}
//=============================================================================
void AppShortcuts::save() const
{
  QSettings settings;
  //qDebug()<<"SAVE: "<<settings.fileName();
  settings.beginGroup("shortcuts");
  settings.remove("");
  settings.beginWriteArray("shortcut");
  int ai=0;
  for(int i=0;i<m_userShortcuts.count();i++){
    const AppShortcut *sc=m_userShortcuts.shortcut(i);
    if(!sc->valid())continue;
    settings.setArrayIndex(ai++);
    settings.setValue("key", sc->key());
    settings.setValue("cmd", sc->cmd());
    settings.setValue("enabled", sc->enabled());
  }
  settings.endArray();
}
//=============================================================================
void AppShortcuts::enableAllSystem(bool v)
{
  for(int i=0;i<m_systemShortcuts.count();i++)m_systemShortcuts.shortcut(i)->setEnabled(v);
}
void AppShortcuts::enableAllUser(bool v)
{
  for(int i=0;i<m_userShortcuts.count();i++)m_userShortcuts.shortcut(i)->setEnabled(v);
}
//=============================================================================
void AppShortcuts::addNew()
{
  if(!m_newItem->valid())return;
  //disable system if any
  for(int i=0;i<m_systemShortcuts.count();i++){
    AppShortcut *sc=m_systemShortcuts.shortcut(i);
    if(!sc->valid())continue;
    if(sc->key()==m_newItem->key()) sc->setEnabled(false);
  }
  //fix exisiting if any
  for(int i=0;i<m_userShortcuts.count();i++){
    AppShortcut *sc=m_userShortcuts.shortcut(i);
    if(!sc->valid())continue;
    if(sc->key()==m_newItem->key()){
      sc->setEnabled(m_newItem->enabled());
      sc->setName(m_newItem->name());
      sc->setCmd(m_newItem->cmd());
      return;
    }
  }
  //add new
  AppShortcut *sc=new AppShortcut(this,widget);
  sc->setEnabled(m_newItem->enabled());
  sc->setName(m_newItem->name());
  sc->setKey(m_newItem->key());
  sc->setCmd(m_newItem->cmd());
  m_userShortcuts.addItem(sc);
  m_newItem->setKey("");
  m_newItem->setCmd("");
}
//=============================================================================
QString AppShortcuts::keyToPortableString(int key,int modifier) const
{
  //qDebug()<<key<<modifier;
  if(key==Qt::Key_Control || key==Qt::Key_Shift || key==Qt::Key_Alt || key==Qt::Key_Meta)
    key=0;
  QKeySequence s(key|modifier);
  return s.toString();
}
//=============================================================================
bool AppShortcuts::blocked() const
{
  return m_blocked;
}
void AppShortcuts::setBlocked(bool v)
{
  if(m_blocked==v)return;
  m_blocked=v;
  for(int i=0;i<m_systemShortcuts.count();i++)m_systemShortcuts.shortcut(i)->updateShortcut();
  for(int i=0;i<m_userShortcuts.count();i++)m_userShortcuts.shortcut(i)->updateShortcut();
  emit blockedChanged();
}
//=============================================================================
AppShortcutModel *AppShortcuts::systemShortcuts()
{
  return &m_systemShortcuts;
}
AppShortcutModel *AppShortcuts::userShortcuts()
{
  return &m_userShortcuts;
}
//=============================================================================
AppShortcut *AppShortcuts::newItem() const
{
  return m_newItem;
}
//=============================================================================
//=============================================================================
AppShortcut::AppShortcut(AppShortcuts *parent, QWidget *widget)
  : QObject(parent), appShortcuts(parent), widget(widget),
    shortcut(NULL), m_enabled(true), m_valid(false)
{
  mandala=qApp->property("Mandala").value<QMandala*>();
  connect(this,SIGNAL(nameChanged()),this,SIGNAL(changed()));
  connect(this,SIGNAL(keyChanged()),this,SIGNAL(changed()));
  connect(this,SIGNAL(cmdChanged()),this,SIGNAL(changed()));
  connect(this,SIGNAL(enabledChanged()),this,SIGNAL(changed()));
}
AppShortcut::~AppShortcut()
{
  if(shortcut)delete shortcut;
}
QString AppShortcut::name() const
{
  return m_name;
}
void AppShortcut::setName(const QString &v)
{
  if(m_name==v)return;
  m_name=v.trimmed();
  emit nameChanged();
}
QString AppShortcut::key() const
{
  return m_key;
}
void AppShortcut::setKey(const QString &v)
{
  if(m_key==v)return;
  m_key=v.trimmed();
  updateShortcut();
  emit keyChanged();
}
QString AppShortcut::cmd() const
{
  return m_cmd;
}
void AppShortcut::setCmd(const QString &v)
{
  if(m_cmd==v)return;
  m_cmd=v.trimmed();
  updateShortcut();
  emit cmdChanged();
}
bool AppShortcut::enabled() const
{
  return m_enabled;
}
void AppShortcut::setEnabled(const bool v)
{
  if(m_enabled==v)return;
  m_enabled=v;
  updateShortcut();
  emit enabledChanged();
}
bool AppShortcut::valid() const
{
  return m_valid;
}
void AppShortcut::setValid(const bool v)
{
  if(m_valid==v)return;
  m_valid=v;
  emit validChanged();
}
//=============================================================================
void AppShortcut::updateShortcut()
{
  if(shortcut){
    delete shortcut;
    shortcut=NULL;
  }
  setValid(!(key().isEmpty() || cmd().isEmpty()));
  if((!m_valid) || (!enabled()) || appShortcuts->blocked() || appShortcuts->newItem()==this)return;
  shortcut=new QShortcut(QKeySequence(key()),widget,0,0,Qt::ApplicationShortcut);
  connect(shortcut,SIGNAL(activated()),this,SLOT(activated()));
}
//=============================================================================
void AppShortcut::activated()
{
  //qDebug()<<qobject_cast<QShortcut*>(sender())->whatsThis();
  mandala->current->exec_script(cmd());
}
//=============================================================================

