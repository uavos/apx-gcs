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
#include "QActionFact.h"
#include <ApxMisc/MaterialIcon.h>

QActionFact::QActionFact(Fact *f, const QColor &iconColor)
    : QAction(f)
    , fact(f)
    , iconColor(iconColor)
{
    if (f->dataType() == Fact::Apply)
        setObjectName("greenAction");
    else if (f->dataType() == Fact::Remove)
        setObjectName("redAction");

    connect(f, &Fact::titleChanged, this, &QActionFact::updateText);
    connect(f, &Fact::descrChanged, this, &QActionFact::updateToolTip);
    connect(f, &Fact::iconChanged, this, &QActionFact::updateIcon);
    connect(f, &Fact::enabledChanged, this, &QActionFact::updateEnabled);
    connect(f, &Fact::visibleChanged, this, &QActionFact::updateVisible);
    connect(f, &Fact::valueChanged, this, &QActionFact::updateChecked);

    connect(this, &QAction::triggered, f, [f]() { f->trigger(); });

    QKeySequence::StandardKey key
        = f->opts().value("shortcut", QKeySequence::UnknownKey).value<QKeySequence::StandardKey>();
    if (key != QKeySequence::UnknownKey)
        setShortcut(key);

    QAction::MenuRole role = f->opts().value("role", QAction::NoRole).value<QAction::MenuRole>();
    if (role != QAction::NoRole)
        setMenuRole(role);

    setCheckable(f->dataType() == Fact::Bool);
    connect(this, &QAction::triggered, this, &QActionFact::actionTriggered);

    updateText();
    updateToolTip();
    updateIcon();
    updateEnabled();
    updateVisible();
    updateChecked();
}

void QActionFact::updateText()
{
    setText(fact->title());
}
void QActionFact::updateToolTip()
{
    setToolTip(fact->descr());
}
void QActionFact::updateIcon()
{
    setIcon(MaterialIcon(fact->icon(), iconColor));
}
void QActionFact::updateEnabled()
{
    setEnabled(fact->enabled());
}
void QActionFact::updateVisible()
{
    setVisible(fact->visible());
}

void QActionFact::updateChecked()
{
    if (!isCheckable())
        return;
    setChecked(fact->value().toBool());
}
void QActionFact::actionTriggered(bool checked)
{
    if (!isCheckable())
        return;
    if (fact->dataType() == Fact::Bool) {
        fact->setValue(checked);
        return;
    }
}
