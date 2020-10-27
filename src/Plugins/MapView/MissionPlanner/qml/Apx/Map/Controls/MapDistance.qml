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
import QtQuick          2.12
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12

import Apx.Common 1.0

RowLayout {

    readonly property int iconSize: text.implicitHeight*0.8


    MaterialIcon {
        name: "chevron-left"
        size: iconSize
    }

    Text {
        id: text
        Layout.fillHeight: true
        Layout.fillWidth: true
        font.family: font_condenced
        color: "#fff"
        property var c: map.mouseCoordinate
        property var c0: map.mouseClickCoordinate
        text: apx.distanceToString(c0.distanceTo(c))
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        ToolTipArea {
            text: qsTr("Distance between points")
        }
    }

    MaterialIcon {
        name: "chevron-right"
        size: iconSize
    }
}
