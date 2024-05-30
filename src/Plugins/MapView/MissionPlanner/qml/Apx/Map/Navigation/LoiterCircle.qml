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
import QtQuick
import QtLocation
import QtPositioning

import Apx.Common

MapCircle {
    color: "#100000FF"
    border.color: "#500000FF"
    border.width: 2

    //Fact bindings
    property real cmd_lat: mandala.cmd.pos.lat.value
    property real cmd_lon: mandala.cmd.pos.lon.value

    property real turnR: mandala.cmd.pos.radius.value
    property int mode: mandala.cmd.reg.pos.value

    center: QtPositioning.coordinate(cmd_lat,cmd_lon)
    radius: Math.abs(turnR)
    visible: mode===reg_pos_loiter

    Behavior on radius { enabled: ui.smooth; NumberAnimation {duration: 100;} }
}
