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
import QtQuick.Controls
import QtQuick.Controls.Material

import Apx.Common

// taskbar badge shown while the datalink blocks all uplink data
Rectangle {
    id: control

    readonly property var fact: apx.datalink.readonly

    readonly property color fillColor: Material.color(Material.Amber)
    readonly property color textColor: "#000"

    implicitHeight: Style.buttonSize
    implicitWidth: row.implicitWidth + height * 0.5

    radius: height / 10
    color: fillColor
    opacity: mouseArea.containsMouse ? 0.85 : 1

    Row {
        id: row
        anchors.centerIn: parent
        height: parent.height
        spacing: control.height * 0.15

        MaterialIcon {
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            size: control.height * 0.8
            name: control.fact.icon
            color: control.textColor
        }
        Text {
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            font: apx.font_narrow(control.height * 0.8, true)
            text: qsTr("READ ONLY")
            color: control.textColor
        }
    }

    // opens the datalink menu, where the read only switch is: a click on the
    // badge must not enable the uplink by itself
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: apx.datalink.trigger()
        ToolTip {
            delay: 1000
            timeout: 5000
            visible: mouseArea.containsMouse
            text: control.fact.title + ": " + control.fact.descr
        }
    }
}
