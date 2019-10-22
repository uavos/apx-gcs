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
#include "Shortcut.h"
#include "Shortcuts.h"

#include <App/App.h>
//=============================================================================
Shortcut::Shortcut(Fact *parent, Shortcuts *shortcuts, const Shortcut *sc, bool bUsr)
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
        _save = new Fact(this, "save", tr("Save"), "", Action | Apply | CloseOnTrigger);
        connect(_save, &Fact::triggered, shortcuts, &Shortcuts::addTriggered);
        defaults();
    } else {
        setSection(bUsr ? shortcuts->f_usr->section() : shortcuts->f_sys->section());
        copyValuesFrom(sc);
        _remove = new Fact(this, "remove", tr("Remove"), "", Action | Remove);
        connect(_remove, &Fact::triggered, this, &Shortcut::remove);
        connect(_remove, &Fact::triggered, shortcuts, &Shortcuts::save);
        connect(shortcuts, &Fact::sizeChanged, this, &Shortcut::updateStats);

        connect(bUsr ? shortcuts->f_allonUsr : shortcuts->f_allonSys,
                &Fact::triggered,
                this,
                &Shortcut::enable);
        connect(bUsr ? shortcuts->f_alloffUsr : shortcuts->f_alloffSys,
                &Fact::triggered,
                this,
                &Shortcut::disable);
        connect(this,
                &Shortcut::titleChanged,
                shortcuts,
                &Shortcuts::updateStats,
                Qt::QueuedConnection);

        connect(this, &Fact::valueChanged, this, [this]() { _enabled->setValue(value()); });
        connect(_enabled, &Fact::valueChanged, this, [this]() { setValue(_enabled->value()); });
        setValue(_enabled->value());
    }

    for (int i = 0; i < size(); ++i) {
        connect(child(i), &Fact::valueChanged, this, &Shortcut::updateStats);
        if (!_new) {
            connect(child(i), &Fact::valueChanged, shortcuts, &Shortcuts::save);
        }
    }
    updateStats();

    App::jsync(this);
}
//=============================================================================
void Shortcut::defaults()
{
    _enabled->setValue(true);
    _key->setValue("");
    _cmd->setValue("");
}
//=============================================================================
void Shortcut::updateStats()
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
void Shortcut::enable()
{
    _enabled->setValue(true);
}
void Shortcut::disable()
{
    _enabled->setValue(false);
}
//=============================================================================
QJsonObject Shortcut::valuesToJson(bool array) const
{
    Q_UNUSED(array)
    QJsonObject jso;
    jso.insert("key", _key->value().toString());
    jso.insert("scr", _cmd->value().toString());
    jso.insert("enb", _enabled->value().toBool());
    return jso;
}
void Shortcut::valuesFromJson(const QJsonObject &jso)
{
    _key->setValue(jso["key"].toVariant());
    _cmd->setValue(jso["scr"].toVariant());
    _enabled->setValue(jso["enb"].toVariant());
}
//=============================================================================
