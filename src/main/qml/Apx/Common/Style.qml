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
    id: singleton

    readonly property real scale: ui.scale

    readonly property real minButtonSize: 20
    readonly property real scaledButtonSize: 26 * scale

    readonly property real widthRatio: 12

    readonly property real buttonSize: Math.max(minButtonSize, scaledButtonSize)

    readonly property real spacing: buttonSize/8
    readonly property real fontSize: buttonSize * 0.6
}
