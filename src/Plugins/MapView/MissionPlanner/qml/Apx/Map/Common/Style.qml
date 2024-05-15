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
pragma Singleton
import QtQuick

QtObject {
    readonly property color cBlue:   "#3779C5"
    readonly property color cYellow: "#ffff00"
    readonly property color cGreen:  "#377964"
    readonly property color cNormal: "#376479"
    readonly property color cRed:    "#793764"

    readonly property color cLineCyan:   "#00FFFF"
    readonly property color cLineGreen:  "#00C800"
    readonly property color cLineRed:    "#FF4C00"
    readonly property color cLineYellow: "#FFC800"
    readonly property color cLineBlue:   "#9696FF"


    readonly property color cWaypoint: "#ffff00"
    readonly property color cRunway:   "#3779C5"
    readonly property color cTaxiway:  "#376479"
    readonly property color cPoint:    "#3779C5"
}
