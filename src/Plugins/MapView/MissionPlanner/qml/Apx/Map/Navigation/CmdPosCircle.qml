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
import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Common 1.0

MapCircle {
    color: "#1000FF00"
    border.color: "magenta"
    border.width: 1

    //Fact bindings
    readonly property real cmd_lat: mandala.cmd.pos.lat.value
    readonly property real cmd_lon: mandala.cmd.pos.lon.value

    property real turnR: mandala.cmd.pos.radius.value
    property bool landing: mandala.cmd.proc.mode.value === proc_mode_LANDING

    center: QtPositioning.coordinate(cmd_lat,cmd_lon)
    radius: Math.max(landing?turnR:0,50)
}
