/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "Shortcut.h"
#include "Shortcuts.h"

#include <App/App.h>

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

    _key = new Fact(this, "key", tr("Key sequence"), "", Text);
    _key->setOpt("editor", "qrc:/" PLUGIN_NAME "/EditorKey.qml");

    _cmd = new Fact(this, "scr", tr("Java script"), "", Text);

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

void Shortcut::defaults()
{
    _enabled->setValue(true);
    _key->setValue("");
    _cmd->setValue("");
}

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

void Shortcut::enable()
{
    _enabled->setValue(true);
}
void Shortcut::disable()
{
    _enabled->setValue(false);
}
