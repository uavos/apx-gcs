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
#include "AppShortcut.h"
#include "AppShortcuts.h"
#include <QShortcut>
#include <QApplication>
#include <QDesktopWidget>
#include "QMandala.h"
//=============================================================================
AppShortcut::AppShortcut(AppShortcuts *parent, const AppShortcut *sc,bool bUsr)
 : Fact(sc?(bUsr?parent->f_usr:parent->f_sys):parent,sc?(bUsr?"usr#":"f_sys#"):tr("add"),sc?"":tr("Add new shortcut"),sc?"":tr("Configure new hotkey"),GroupItem,sc?BoolData:NoData),
   container(parent),_new(sc?false:true),bUsr(bUsr), shortcut(NULL)
{
  mandala=qApp->property("Mandala").value<QMandala*>();

  _enabled=new Fact(this,"enabled",tr("Enabled"),tr("Connectwhen available"),FactItem,BoolData);
  if(!_new)bindValue(_enabled);

  _key=new Fact(this,"key",tr("Key sequence"),"",FactItem,KeySequenceData);
  _cmd=new Fact(this,"Command",tr("Java script"),"",FactItem,TextData);

  if(_new){
    _save=new Fact(this,"save",tr("Save"),"",FactItem,ActionData);
    connect(_save,&Fact::triggered,parent,&AppShortcuts::addTriggered);
    defaults();
  }else{
    setSection(bUsr?parent->f_usr->section():parent->f_sys->section());
    copyValuesFrom(sc);
    _remove=new Fact(this,"remove",tr("Remove"),"",FactItem,ActionData);
    connect(_remove,&Fact::triggered,parent,&AppShortcuts::removeTriggered);
    _remove->setValue(RemoveAction);
    connect(parent,&Fact::structChanged,this,&AppShortcut::updateStats);
    connect(parent->f_blocked,&Fact::valueChanged,this,&AppShortcut::updateShortcut,Qt::QueuedConnection);
    connect(this,&Fact::childValueChanged,this,&AppShortcut::updateShortcut,Qt::QueuedConnection);

    connect(bUsr?parent->f_allonUsr:parent->f_allonSys,&Fact::triggered,this,&AppShortcut::enable);
    connect(bUsr?parent->f_alloffUsr:parent->f_alloffSys,&Fact::triggered,this,&AppShortcut::disable);
    updateShortcut();
  }

  connect(this,&Fact::childValueChanged,this,&AppShortcut::updateStats);
  updateStats();
}
AppShortcut::~AppShortcut()
{
  if(shortcut)delete shortcut;
}
//=============================================================================
void AppShortcut::defaults()
{
  _enabled->setValue(true);
  _key->setValue("");
  _cmd->setValue("");
}
//=============================================================================
void AppShortcut::updateStats()
{
  if(_new){
    _save->setEnabled(!(_key->text().isEmpty()||_cmd->text().isEmpty()));
  }else{
    _remove->setVisible(bUsr);
    setTitle(QString("%1 -> %2").arg(_key->text()).arg(_cmd->text()));
    _key->setEnabled(bUsr);
    _cmd->setEnabled(bUsr);
  }
}
//=============================================================================
void AppShortcut::updateShortcut()
{
  if(shortcut){
    delete shortcut;
    shortcut=NULL;
  }
  bool valid=(!(_key->text().isEmpty() || _cmd->text().isEmpty()));
  if((!valid) || (!_enabled->value().toBool()) || container->f_blocked->value().toBool() || _new)return;
  shortcut=new QShortcut(QKeySequence(_key->text()),container->widget,0,0,Qt::ApplicationShortcut);
  connect(shortcut,&QShortcut::activated,this,&AppShortcut::shortcutActivated);
  //qDebug()<<shortcut;
}
//=============================================================================
void AppShortcut::shortcutActivated()
{
  //qDebug()<<sender();//qobject_cast<QShortcut*>(sender())->whatsThis();
  mandala->current->exec_script(_cmd->text());
}
//=============================================================================
void AppShortcut::enable()
{
  _enabled->setValue(true);
}
void AppShortcut::disable()
{
  _enabled->setValue(false);
}
//=============================================================================

