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
  setFlatModel(true);

  setSection(FactSystem::ApplicationSection);

  // QML types register
  qmlRegisterUncreatableType<AppShortcuts>("GCS.AppShortcuts", 1, 0, "AppShortcuts", "Reference only");

  f_add=new AppShortcut(this,NULL,false);
  f_blocked=new Fact(this,"blocked",tr("Block all"),tr("Temporally block all shortcuts"),FactItem,BoolData);
  //f_blocked->setVisible(false);

  QString sect;
  sect=tr("User");

  f_allonUsr=new Fact(this,"allonUsr",tr("Enable all"),tr("Turn on all shortcuts"),FactItem,NoData);
  f_allonUsr->setSection(sect);
  f_alloffUsr=new Fact(this,"alloffUsr",tr("Disable all"),tr("Turn off all shortcuts"),FactItem,NoData);
  f_alloffUsr->setSection(sect);
  f_usr=new Fact(this,"user",sect,tr("User defined shortcuts"),SectionItem,NoData);
  f_usr->setSection(sect);


  sect=tr("System");
  f_allonSys=new Fact(this,"allonSys",tr("Enable all"),tr("Turn on all shortcuts"),FactItem,NoData);
  f_allonSys->setSection(sect);
  f_alloffSys=new Fact(this,"alloffSys",tr("Disable all"),tr("Turn off all shortcuts"),FactItem,NoData);
  f_alloffSys->setSection(sect);
  f_sys=new Fact(this,"f_system",sect,tr("System default shortcuts"),SectionItem,NoData);
  f_sys->setSection(sect);

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
void AppShortcuts::updateStats()
{
  bool bSz=f_sys->size();
  f_allonSys->setEnabled(bSz);
  f_alloffSys->setEnabled(bSz);
  bSz=f_usr->size();
  f_allonUsr->setEnabled(bSz);
  f_alloffUsr->setEnabled(bSz);
  //disable f_sys shortcuts found in user
  foreach (FactTree *i, f_usr->childItems()) {
    AppShortcut *item=static_cast<AppShortcut*>(i);
    if(item->_enabled->value().toBool()==false)continue;
    foreach (FactTree *i, f_sys->childItems()) {
      AppShortcut *f_sys=static_cast<AppShortcut*>(i);
      if(f_sys->_enabled->value().toBool()==false)continue;
      if(f_sys->_key->text()==item->_key->text()){
        qWarning("%s: %s",tr("Duplicate shortcut").toUtf8().data(),f_sys->_key->text().toUtf8().data());
        item->_enabled->setValue(false);
      }
    }
  }

}
//=============================================================================
void AppShortcuts::addTriggered()
{
  addUserShortcut();
  save();
  f_add->defaults();
}
void AppShortcuts::removeTriggered()
{
  f_usr->removeItem(static_cast<FactTree*>(sender())->parentItem());
  save();
}
//=============================================================================
void AppShortcuts::addUserShortcut()
{
  new AppShortcut(this,f_add,true);
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
    f_add->defaults();
    f_add->_key->setValue(key);
    f_add->_cmd->setValue(cmd);
    new AppShortcut(this,f_add,false);
  }

  QSettings *settings=AppSettings::settings();
  settings->beginGroup(path());
  //read usr
  int size=settings->beginReadArray("usr");
  for (int i = 0; i < size; ++i) {
    settings->setArrayIndex(i);
    f_add->defaults();
    foreach(FactTree *i,f_add->childItems()){
      Fact *fact=static_cast<Fact*>(i);
      if(fact->dataType()==ActionData)continue;
      fact->setValue(settings->value(fact->name()));
    }
    addUserShortcut();
  }
  settings->endArray();
  //read f_sys enable/disable
  size=settings->beginReadArray("f_sys");
  for (int i = 0; i < size; ++i) {
    settings->setArrayIndex(i);
    foreach (FactTree *i, f_sys->childItems()) {
      AppShortcut *item=static_cast<AppShortcut*>(i);
      if(item->_key->value().toString()!=settings->value(item->_key->name()).toString())continue;
      if(item->_cmd->value().toString()!=settings->value(item->_cmd->name()).toString())continue;
      if(settings->value(item->_enabled->name()).toBool())continue;
      item->_enabled->setValue(false);
    }
  }
  //close
  f_add->defaults();
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
  foreach (FactTree *i, f_usr->childItems()) {
    AppShortcut *item=static_cast<AppShortcut*>(i);
    settings->setArrayIndex(ai++);
    foreach(const FactTree *i,item->childItems()){
      const Fact *fact=static_cast<const Fact*>(i);
      if((!fact->visible())||fact->dataType()==ActionData)continue;
      settings->setValue(fact->name(),fact->text());
    }
  }
  settings->endArray();
  //save f_sys enable/disable
  settings->beginWriteArray("f_sys");
  ai=0;
  foreach (FactTree *i, f_sys->childItems()) {
    AppShortcut *item=static_cast<AppShortcut*>(i);
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

