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
#include "AppSettings.h"
#include "AppShortcuts.h"
#include "AppShortcut.h"
#include "AppDirs.h"
//=============================================================================
AppShortcuts::AppShortcuts(FactSystem *parent, QWidget *widget)
  : Fact(parent->tree(),"shortcuts",tr("Shortcuts"),tr("Keyboard hotkeys"),RootItem,NoData),
    widget(widget)
{
  setSection(FactSystem::ApplicationSection);

  // QML types register
  qmlRegisterUncreatableType<AppShortcuts>("GCS.AppShortcuts", 1, 0, "AppShortcuts", "Reference only");

  _add=new AppShortcut(this);
  _blocked=new Fact(this,"blocked",tr("Block all"),tr("Temporally block all shortcuts"),FactItem,BoolData);
  //_blocked->setVisible(false);

  QString sect;
  sect=tr("User");
  usrSect=sect;

  _allonUsr=new Fact(this,"allon",tr("Enable all"),tr("Turn on all shortcuts"),FactItem,NoData);
  _allonUsr->setSection(sect);
  connect(_allonUsr,&Fact::triggered,this,&AppShortcuts::allonUsrTriggered);
  _alloffUsr=new Fact(this,"alloff",tr("Disable all"),tr("Turn off all shortcuts"),FactItem,NoData);
  _alloffUsr->setSection(sect);
  connect(_alloffUsr,&Fact::triggered,this,&AppShortcuts::alloffUsrTriggered);

  sect=tr("System");
  sysSect=sect;
  _allonSys=new Fact(this,"allon",tr("Enable all"),tr("Turn on all shortcuts"),FactItem,NoData);
  _allonSys->setSection(sect);
  connect(_allonSys,&Fact::triggered,this,&AppShortcuts::allonSysTriggered);
  _alloffSys=new Fact(this,"alloff",tr("Disable all"),tr("Turn off all shortcuts"),FactItem,NoData);
  _alloffSys->setSection(sect);
  connect(_alloffSys,&Fact::triggered,this,&AppShortcuts::alloffSysTriggered);

  load();

  saveTimer.setSingleShot(true);
  saveTimer.setInterval(500);
  connect(&saveTimer,&QTimer::timeout,this,&AppShortcuts::save);
  connect(this,SIGNAL(childValueChanged()),&saveTimer,SLOT(start()));

  connect(this,&Fact::structChanged,this,&AppShortcuts::updateStats);
  connect(this,&Fact::childValueChanged,this,&AppShortcuts::updateStats);
  updateStats();
}
//=============================================================================
void AppShortcuts::allonSysTriggered()
{
  foreach (AppShortcut *item, sysList) {
    item->_enabled->setValue(true);
  }
}
void AppShortcuts::alloffSysTriggered()
{
  foreach (AppShortcut *item, sysList) {
    item->_enabled->setValue(false);
  }
}
void AppShortcuts::allonUsrTriggered()
{
  foreach (AppShortcut *item, usrList) {
    item->_enabled->setValue(true);
  }
}
void AppShortcuts::alloffUsrTriggered()
{
  foreach (AppShortcut *item, usrList) {
    item->_enabled->setValue(false);
  }
}
//=============================================================================
void AppShortcuts::updateStats()
{
  bool bSz=sysList.size();
  _allonSys->setEnabled(bSz);
  _alloffSys->setEnabled(bSz);
  bSz=usrList.size();
  _allonUsr->setEnabled(bSz);
  _alloffUsr->setEnabled(bSz);
  //disable sys shortcuts found in user
  foreach (AppShortcut *item, usrList) {
    if(item->_enabled->value().toBool()==false)continue;
    foreach (AppShortcut *sys, sysList) {
      if(sys->_enabled->value().toBool()==false)continue;
      if(sys->_key->text()==item->_key->text()){
        qWarning("%s: %s",tr("Duplicate shortcut").toUtf8().data(),sys->_key->text().toUtf8().data());
        item->_enabled->setValue(false);
      }
    }
  }

}
//=============================================================================
void AppShortcuts::addTriggered()
{
  addUserShortcut();
  updateStats();
  save();
  _add->defaults();
}
void AppShortcuts::removeTriggered()
{
  AppShortcut *item=static_cast<AppShortcut*>(static_cast<FactTree*>(sender())->parentItem());
  usrList.removeAll(item);
  removeItem(item);
  save();
}
//=============================================================================
void AppShortcuts::addUserShortcut()
{
  AppShortcut *item=new AppShortcut(this,_add,true);
  moveItem(item,_allonSys->num());
  item->setSection(usrSect);
  usrList.append(item);
}
//=============================================================================
void AppShortcuts::load()
{
  QSettings settingsSystem(AppDirs::res().filePath("templates/shortcuts.conf"),QSettings::IniFormat);
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
    _add->defaults();
    _add->_key->setValue(key);
    _add->_cmd->setValue(cmd);
    AppShortcut *item=new AppShortcut(this,_add);
    item->setSection(sysSect);
    sysList.append(item);
  }

  QSettings *settings=AppSettings::settings();
  settings->beginGroup(path());
  //read usr
  int size=settings->beginReadArray("usr");
  for (int i = 0; i < size; ++i) {
    settings->setArrayIndex(i);
    _add->defaults();
    foreach(FactTree *i,_add->childItems()){
      Fact *fact=static_cast<Fact*>(i);
      if(fact->dataType()==ActionData)continue;
      fact->setValue(settings->value(fact->name()));
    }
    addUserShortcut();
  }
  settings->endArray();
  //read sys enable/disable
  size=settings->beginReadArray("sys");
  for (int i = 0; i < size; ++i) {
    settings->setArrayIndex(i);
    foreach (AppShortcut *item, sysList) {
      if(item->_key->value().toString()!=settings->value(item->_key->name()).toString())continue;
      if(item->_cmd->value().toString()!=settings->value(item->_cmd->name()).toString())continue;
      if(settings->value(item->_enabled->name()).toBool())continue;
      item->_enabled->setValue(false);
    }
  }
  //close
  _add->defaults();
  settings->endArray();
  settings->endGroup();
}
void AppShortcuts::save()
{
  QSettings *settings=AppSettings::settings();
  settings->beginGroup(path());
  settings->remove(""); //all
  //save usr
  settings->beginWriteArray("usr");
  int ai=0;
  foreach(const AppShortcut *item,usrList){
    settings->setArrayIndex(ai++);
    foreach(const FactTree *i,item->childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      if((!fact->visible())||fact->dataType()==ActionData)continue;
      settings->setValue(fact->name(),fact->text());
    }
  }
  settings->endArray();
  //save sys enable/disable
  settings->beginWriteArray("sys");
  ai=0;
  foreach(const AppShortcut *item,sysList){
    if(item->_enabled->value().toBool())continue;
    settings->setArrayIndex(ai++);
    foreach(const FactTree *i,item->childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      if((!fact->visible())||fact->dataType()==ActionData)continue;
      settings->setValue(fact->name(),fact->text());
    }
  }
  settings->endArray();
  settings->endGroup();
  //qDebug()<<"saved";
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

