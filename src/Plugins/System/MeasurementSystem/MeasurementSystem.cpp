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
#include "MeasurementSystem.h"
#include <App/App.h>
#include <Fact/Fact.h>

MeasurementSystem::MeasurementSystem(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Measurement system"),
           tr("Select and settup a measurement system"),
           Group)
{
    f_feets = new Fact(this,
                          "feets",
                          tr("Use feets"),
                          tr("Use US customary measurement system"),
                          Bool | PersistentValue);

    f_tooltip = new Fact(this,
                         "tooltip",
                         tr("Show tooltip"),
                         tr("Show tooltip in other units of measurement"),
                         Bool | PersistentValue);
}
