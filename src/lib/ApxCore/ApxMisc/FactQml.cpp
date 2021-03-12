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
#include "FactQml.h"

FactQml::FactQml(QObject *parent)
    : Fact(parent)
{}

QQmlListProperty<FactQml> FactQml::children()
{
    return QQmlListProperty<FactQml>(this,
                                     this,
                                     &FactQml::appendChildren,
                                     &FactQml::countChildren,
                                     &FactQml::atChildren,
                                     &FactQml::clearChildren);
}

void FactQml::appendChildren(QQmlListProperty<FactQml> *property, FactQml *value)
{
    value->setParentFact(reinterpret_cast<Fact *>(property->data));
}
FactQml *FactQml::atChildren(QQmlListProperty<FactQml> *property, int index)
{
    return reinterpret_cast<FactQml *>(reinterpret_cast<Fact *>(property->data)->child(index));
}
void FactQml::clearChildren(QQmlListProperty<FactQml> *property)
{
    reinterpret_cast<Fact *>(property->data)->deleteChildren();
}
int FactQml::countChildren(QQmlListProperty<FactQml> *property)
{
    return reinterpret_cast<Fact *>(property->data)->size();
}
