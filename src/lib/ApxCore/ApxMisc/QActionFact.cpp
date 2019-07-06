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
#include "QActionFact.h"
#include <ApxMisc/SvgMaterialIcon.h>
//=============================================================================
QActionFact::QActionFact(Fact *f)
    : QAction(f)
    , fact(f)
    , act(nullptr)
{
    connect(f, &Fact::titleChanged, this, &QActionFact::updateText);
    connect(f, &Fact::descrChanged, this, &QActionFact::updateToolTip);
    connect(f, &Fact::iconChanged, this, &QActionFact::updateIcon);
    connect(f, &Fact::enabledChanged, this, &QActionFact::updateEnabled);
    connect(f, &Fact::visibleChanged, this, &QActionFact::updateVisible);

    connect(this, &QAction::triggered, f, &Fact::trigger);

    updateText();
    updateToolTip();
    updateIcon();
    updateEnabled();
    updateVisible();
}
//=============================================================================
QActionFact::QActionFact(FactAction *f)
    : QAction(f)
    , fact(nullptr)
    , act(f)
{
    if (f->flags() & FactAction::ActionApply)
        setObjectName("greenAction");
    else if (f->flags() & FactAction::ActionRemove)
        setObjectName("redAction");

    connect(f, &FactAction::titleChanged, this, &QActionFact::updateText);
    connect(f, &FactAction::descrChanged, this, &QActionFact::updateToolTip);
    connect(f, &FactAction::iconChanged, this, &QActionFact::updateIcon);
    connect(f, &FactAction::enabledChanged, this, &QActionFact::updateEnabled);
    connect(f, &FactAction::visibleChanged, this, &QActionFact::updateVisible);

    connect(this, &QAction::triggered, f, &FactAction::trigger);

    updateText();
    updateToolTip();
    updateIcon();
    updateEnabled();
    updateVisible();
}
//=============================================================================
void QActionFact::updateText()
{
    setText(fact ? fact->title() : act->title());
}
void QActionFact::updateToolTip()
{
    setToolTip(fact ? fact->descr() : act->descr());
}
void QActionFact::updateIcon()
{
    setIcon(SvgMaterialIcon(fact ? fact->icon() : act->icon()));
}
void QActionFact::updateEnabled()
{
    setEnabled(fact ? fact->enabled() : act->enabled());
}
void QActionFact::updateVisible()
{
    setVisible(fact ? fact->visible() : act->visible());
}
//=============================================================================
//=============================================================================
//=============================================================================
