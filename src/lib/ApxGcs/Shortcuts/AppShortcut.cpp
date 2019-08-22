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
#include "AppShortcut.h"
#include "AppShortcuts.h"

#include <App/AppSettings.h>
#include <ApxApp.h>
//=============================================================================
AppShortcut::AppShortcut(Fact *parent, AppShortcuts *shortcuts, const AppShortcut *sc, bool bUsr)
    : Fact(parent,
           sc ? (bUsr ? "usr#" : "sys#") : tr("add"),
           sc ? "" : tr("Add new shortcut"),
           sc ? "" : tr("Configure new hotkey"),
           Group | (sc ? Bool : NoFlags))
    , container(shortcuts)
    , _new(sc ? false : true)
    , bUsr(bUsr)
{
    _enabled = new Fact(this, "enb", tr("Enabled"), "", Bool);

    _key = new Fact(this, "key", tr("Key sequence"), "", Key);
    _cmd = new Fact(this, "jscmd", tr("Java script"), "", Text);

    if (_new) {
        _save = new FactAction(this,
                               "save",
                               tr("Save"),
                               "",
                               "",
                               FactAction::ActionApply | FactAction::ActionCloseOnTrigger);
        connect(_save, &FactAction::triggered, shortcuts, &AppShortcuts::addTriggered);
        defaults();
    } else {
        setSection(bUsr ? shortcuts->f_usr->section() : shortcuts->f_sys->section());
        copyValuesFrom(sc);
        _remove = new FactAction(this, "remove", tr("Remove"), "", "", FactAction::ActionRemove);
        connect(_remove, &FactAction::triggered, this, &AppShortcut::remove);
        connect(_remove, &FactAction::triggered, shortcuts, &AppShortcuts::save);
        connect(shortcuts, &Fact::sizeChanged, this, &AppShortcut::updateStats);

        connect(bUsr ? shortcuts->f_allonUsr : shortcuts->f_allonSys,
                &Fact::triggered,
                this,
                &AppShortcut::enable);
        connect(bUsr ? shortcuts->f_alloffUsr : shortcuts->f_alloffSys,
                &Fact::triggered,
                this,
                &AppShortcut::disable);
        connect(this,
                &AppShortcut::titleChanged,
                shortcuts,
                &AppShortcuts::updateStats,
                Qt::QueuedConnection);

        connect(this, &Fact::valueChanged, this, [this]() { _enabled->setValue(value()); });
        connect(_enabled, &Fact::valueChanged, this, [this]() { setValue(_enabled->value()); });
        setValue(_enabled->value());
    }

    /*if(!sc)shortcuts->addItem(this);
  else if(bUsr){
    shortcuts->f_usr->addItem(this);
  }else shortcuts->f_sys->addItem(this);*/

    for (int i = 0; i < size(); ++i) {
        connect(child(i), &Fact::valueChanged, this, &AppShortcut::updateStats);
        if (!_new) {
            connect(child(i), &Fact::valueChanged, shortcuts, &AppShortcuts::save);
        }
    }
    updateStats();

    ApxApp::jsync(this);
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
    if (_new) {
        _save->setEnabled(!(_key->text().isEmpty() || _cmd->text().isEmpty()));
    } else {
        _remove->setVisible(bUsr);
        setTitle(QString("%1 -> %2").arg(_key->text()).arg(_cmd->text()));
        _key->setEnabled(bUsr);
        _cmd->setEnabled(bUsr);
    }
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
QJsonObject AppShortcut::valuesToJson(bool array) const
{
    Q_UNUSED(array)
    QJsonObject jso;
    jso.insert("key", _key->value().toString());
    jso.insert("scr", _cmd->value().toString());
    jso.insert("enb", _enabled->value().toBool());
    return jso;
}
void AppShortcut::valuesFromJson(const QJsonObject &jso)
{
    _key->setValue(jso["key"].toVariant());
    _cmd->setValue(jso["scr"].toVariant());
    _enabled->setValue(jso["enb"].toVariant());
}
//=============================================================================
