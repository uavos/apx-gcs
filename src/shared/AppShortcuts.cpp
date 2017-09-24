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
    :QObject(parent),widget(parent)
{
  qmlRegisterType<AppShortcuts>();
  qmlRegisterType<AppShortcut>();
  load();
  qApp->setProperty("shortcuts",qVariantFromValue(this));
}
//=============================================================================
void AppShortcuts::load()
{
  QSettings stt(QMandala::Global::config().filePath("shortcuts.conf"),QSettings::IniFormat);
  foreach(QString key,stt.childKeys()){
    if(key.trimmed().startsWith("#"))continue;
    if(key.trimmed().isEmpty())continue;
    QString text_command;
    if(!key.contains("shortcut")){
      text_command=stt.value(key).toString();
    }else{
      QStringList st=stt.value(key).toStringList();
      if(st.size()!=2)continue;
      key=st.at(0);
      text_command=st.at(1);
    }
    /*QShortcut *sc;
    if(key.toLower().contains("ctrl")){
      sc=new QShortcut(QKeySequence(key),widget,0,0,Qt::ApplicationShortcut);
      shortcuts.append(sc);
      sc->setWhatsThis(text_command);
      connect(sc,SIGNAL(activated()),this,SLOT(shortcutActivated()));
    }else{
      sc=new QShortcut(QKeySequence(key),widget,0,0,Qt::ApplicationShortcut);
      shortcuts.append(sc);
      sc->setWhatsThis(text_command);
      connect(sc,SIGNAL(activated()),this,SLOT(shortcutActivated()));
      sc=new QShortcut(QKeySequence("Ctrl+"+key),widget,0,0,Qt::ApplicationShortcut);
      shortcuts.append(sc);
      sc->setWhatsThis(text_command);
      connect(sc,SIGNAL(activated()),this,SLOT(shortcutActivated()));
    }*/
    AppShortcut *sc=new AppShortcut(this);
    sc->setKey(key);
    sc->setCmd(text_command);
    sc->updateShortcut(widget);
    shortcuts.append(sc);
  }
  emit itemsChanged();
}
//=============================================================================
void AppShortcuts::save()
{
}
//=============================================================================
QQmlListProperty<AppShortcut> AppShortcuts::items()
{
  return QQmlListProperty<AppShortcut>(this, this,
           &AppShortcuts::appendItem,
           &AppShortcuts::itemCount,
           &AppShortcuts::item,
           &AppShortcuts::clearItems);
}
//=============================================================================
void AppShortcuts::appendItem(QQmlListProperty<AppShortcut>*list, AppShortcut* sc)
{
  AppShortcuts *p=reinterpret_cast<AppShortcuts*>(list->data);
  p->shortcuts.append(sc);
}
int AppShortcuts::itemCount(QQmlListProperty<AppShortcut>*list)
{
  return reinterpret_cast<AppShortcuts*>(list->data)->shortcuts.count();
}
AppShortcut* AppShortcuts::item(QQmlListProperty<AppShortcut>* list, int i)
{
  return reinterpret_cast<AppShortcuts*>(list->data)->shortcuts.at(i);
}
void AppShortcuts::clearItems(QQmlListProperty<AppShortcut>* list)
{
  reinterpret_cast<AppShortcuts*>(list->data)->shortcuts.clear();
}
//=============================================================================
//=============================================================================
AppShortcut::AppShortcut(QObject *parent)
  : QObject(parent),shortcut(NULL)
{
  mandala=qApp->property("Mandala").value<QMandala*>();
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
  m_name=v;
  emit changed();
}
QString AppShortcut::key() const
{
  return m_key;
}
void AppShortcut::setKey(const QString &v)
{
  if(m_key==v)return;
  m_key=v;
  updateShortcut(NULL);
  emit changed();
}
QString AppShortcut::cmd() const
{
  return m_cmd;
}
void AppShortcut::setCmd(const QString &v)
{
  if(m_cmd==v)return;
  m_cmd=v;
  updateShortcut(NULL);
  emit changed();
}
void AppShortcut::updateShortcut(QWidget *widget)
{
  if(!widget){
    if(shortcut)widget=shortcut->parentWidget();
    else return;
  }
  if(shortcut)delete shortcut;
  shortcut=new QShortcut(QKeySequence(key()),widget,0,0,Qt::ApplicationShortcut);
  shortcut->setWhatsThis(cmd());
  connect(shortcut,SIGNAL(activated()),this,SLOT(activated()));
}
void AppShortcut::activated()
{
  //qDebug()<<qobject_cast<QShortcut*>(sender())->whatsThis();
  mandala->current->exec_script(qobject_cast<QShortcut*>(sender())->whatsThis());
}
//=============================================================================

