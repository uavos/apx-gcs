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
    AppShortcut *sc=new AppShortcut(this,widget);
    sc->setKey(key);
    sc->setCmd(text_command);
    m_systemShortcuts.addItem(sc);
  }
  m_newItem=new AppShortcut(this,widget);
  //m_userShortcuts.addItem(m_newItem);
  emit shortcutsChanged();
  connect(&m_userShortcuts,SIGNAL(changed()),this,SIGNAL(shortcutsChanged()));
}
//=============================================================================
void AppShortcuts::save()
{
}
//=============================================================================
void AppShortcuts::enableAllSystem(bool v)
{
  for(int i=0;i<m_systemShortcuts.rowCount();i++)m_systemShortcuts.item(i)->setEnabled(v);
}
void AppShortcuts::enableAllUser(bool v)
{
  for(int i=0;i<m_userShortcuts.rowCount();i++)m_userShortcuts.item(i)->setEnabled(v);
}
//=============================================================================
void AppShortcuts::addNew()
{
  if(!m_newItem->valid())return;
  AppShortcut *sc=new AppShortcut(this,widget);
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
  for(int i=0;i<m_systemShortcuts.rowCount();i++)m_systemShortcuts.item(i)->updateShortcut();
  for(int i=0;i<m_userShortcuts.rowCount();i++)m_userShortcuts.item(i)->updateShortcut();
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
AppShortcutModel::AppShortcutModel(QObject *parent)
  : QAbstractListModel(parent)
{
}
void AppShortcutModel::addItem(AppShortcut *item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  m_items << item;
  endInsertRows();
  emit changed();
}
void AppShortcutModel::removeItem(AppShortcut *item)
{
  //qDebug()<<item;
  int i=m_items.indexOf(item);
  if(i<0)return;
  beginRemoveRows(QModelIndex(), i, i);
  m_items.removeOne(item);
  endRemoveRows();
  item->deleteLater();
}
int AppShortcutModel::rowCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent);
  return m_items.count();
}
QVariant AppShortcutModel::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() >= m_items.count())
    return QVariant();
  if (role == ItemRole) return QVariant::fromValue(m_items[index.row()]);
  return QVariant();
}
QHash<int, QByteArray> AppShortcutModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[ItemRole] = "item";
  return roles;
}
AppShortcut *AppShortcutModel::item(int row)
{
  if(row>m_items.count())return NULL;
  return m_items[row];
}
//=============================================================================
//=============================================================================
AppShortcut::AppShortcut(AppShortcuts *parent, QWidget *widget)
  : QObject(parent), appShortcuts(parent), widget(widget),
    shortcut(NULL), m_enabled(true), m_valid(false)
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

