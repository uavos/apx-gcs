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
#pragma once

#include "UnitMission.h"
#include <QtCore>

class AirspaceItem : public Fact
{
    Q_OBJECT

public:
    explicit AirspaceItem(Fact *parent);

    Fact *f_role;
    Fact *f_shape;
    Fact *f_top;
    Fact *f_bottom;
    Fact *f_inverted;

    Fact *f_label;

    Fact *f_points;

private slots:
    void updateTitle();
    void updateDescr();
};
