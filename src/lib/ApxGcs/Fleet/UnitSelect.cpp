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
#include "UnitSelect.h"
#include "Fleet.h"
#include "Unit.h"

UnitSelect::UnitSelect(Fact *parent, const QString &name, const QString &title, const QString &descr)
    : Fact(parent, name, title, descr, Group)
    , fleet(Fleet::instance())
{
    setEnabled(false);
    connect(fleet, &Fleet::unitRegistered, this, &UnitSelect::addUnit);
    connect(fleet, &Fleet::unitRemoved, this, &UnitSelect::_unitRemoved);
    connect(fleet, &Fleet::unitSelected, this, &UnitSelect::_unitSelected);
}

void UnitSelect::addUnit(Unit *unit)
{
    auto f = new Fact(this, unit->name(), unit->title(), unit->descr());
    map.insert(unit, f);

    connect(f, &Fact::triggered, this, &UnitSelect::_factTriggered);

    f->bindProperty(unit, "visible", true);
    f->bindProperty(unit, "active", true);
    f->bindProperty(unit, "value", true);
    f->bindProperty(unit, "icon", true);

    setEnabled(true);
}

void UnitSelect::_unitRemoved(Unit *unit)
{
    auto f = map.value(unit);
    if (!f)
        return;
    map.remove(unit);
    f->deleteFact();
    setEnabled(size() > 0);
}

void UnitSelect::_unitSelected(Unit *unit)
{
    setValue(unit->title());
}

void UnitSelect::_factTriggered()
{
    auto v = map.key(qobject_cast<Fact *>(sender()));
    if (v)
        emit unitSelected(v);
}
