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

    connect(this, &QAction::triggered, f, &Fact::trigger);

    updateText();
    updateToolTip();
    updateIcon();
    updateEnabled();
    updateVisible();
}
//=============================================================================
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
    setIcon(SvgMaterialIcon(fact->icon()));
}
void QActionFact::updateEnabled()
{
    setEnabled(fact->enabled());
}
void QActionFact::updateVisible()
{
    setVisible(fact->visible());
}
//=============================================================================
//=============================================================================
//=============================================================================
