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
import QtQuick.Controls.Material

import Apx.Common

/*

    Drag handle placed over a layout separator line (the parent item).
    Shown in the layout edit mode only.

*/

Item {
    id: handle

    // true for a horizontal line dragged up and down
    property bool vertical: false
    property bool active: false

    signal started()
    signal moved(real delta) // offset from the press point
    signal reset()

    readonly property real grip: Math.max(5, Style.spacing*1.5)

    visible: active
    anchors.fill: parent
    anchors.leftMargin: vertical?0:-grip
    anchors.rightMargin: vertical?0:-grip
    anchors.topMargin: vertical?-grip:0
    anchors.bottomMargin: vertical?-grip:0

    readonly property color color: (mouse.pressed || mouse.containsMouse)
                                   ? Material.color(Material.Amber)
                                   : Material.color(Material.Cyan)

    Rectangle {
        anchors.centerIn: parent
        width: vertical?parent.width:3
        height: vertical?3:parent.height
        border.width: 0
        color: handle.color
        opacity: 0.7
    }
    Rectangle {
        anchors.centerIn: parent
        width: vertical?grip*8:grip*2
        height: vertical?grip*2:grip*8
        radius: grip
        border.width: 0
        color: handle.color
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        cursorShape: vertical?Qt.SizeVerCursor:Qt.SizeHorCursor

        // scene coordinates, the handle itself moves while dragging
        property point p0
        onPressed: (event) => {
            p0=mapToItem(null, event.x, event.y)
            handle.started()
        }
        onPositionChanged: (event) => {
            if(!pressed)
                return
            var p=mapToItem(null, event.x, event.y)
            handle.moved(vertical?(p.y-p0.y):(p.x-p0.x))
        }
        onDoubleClicked: handle.reset()
    }
}
