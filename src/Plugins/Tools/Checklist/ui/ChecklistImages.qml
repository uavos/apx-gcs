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
import QtQuick.Layouts

import Apx.Common

// One image group of a step body, centered: a single image as is, several
// images as a slider with left/right arrows and a position counter.
ColumnLayout {
    id: control

    property var urls: []    // image sources of the group

    // own theme instance: one passed in from outside is not yet set when the
    // bindings of a Repeater delegate are first evaluated
    ChecklistTheme { id: theme }

    property int current: 0
    onUrlsChanged: current = 0
    readonly property bool slider: urls.length > 1

    readonly property real sp: theme.sp
    readonly property real fs: theme.fs

    spacing: sp

    function go(step)
    {
        current = (current + step + urls.length) % urls.length
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: img.height

        Image {
            id: img
            anchors.horizontalCenter: parent.horizontalCenter
            // natural size; only scaled down when wider than the pane
            width: Math.min(parent.width, sourceSize.width > 0 ? sourceSize.width : parent.width)
            height: sourceSize.width > 0 ? width * sourceSize.height / sourceSize.width : 0
            fillMode: Image.PreserveAspectFit
            source: control.urls.length ? control.urls[Math.min(control.current, control.urls.length - 1)] : ""
            asynchronous: true
        }
        Rectangle {
            anchors.fill: img
            visible: img.status === Image.Ready
            color: "transparent"
            border.color: theme.panelBorder
            border.width: 1
        }

        // slider arrows over the image sides
        Repeater {
            model: control.slider ? [-1, 1] : []
            Rectangle {
                readonly property int dir: modelData
                anchors.verticalCenter: img.verticalCenter
                x: dir < 0 ? img.x + control.sp : img.x + img.width - width - control.sp
                width: control.fs*3
                height: width
                radius: width/2
                color: arrowHover.containsMouse ? "#d0000000" : "#90000000"
                border.color: "#60ffffff"
                border.width: 1
                MaterialIcon {
                    anchors.centerIn: parent
                    name: parent.dir < 0 ? "chevron-left" : "chevron-right"
                    size: control.fs*2.4
                    color: "#fff"
                }
                MouseArea {
                    id: arrowHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: control.go(parent.dir)
                }
            }
        }
    }

    // position in the group
    Text {
        Layout.alignment: Qt.AlignHCenter
        visible: control.slider
        text: (control.current + 1) + " / " + control.urls.length
        color: theme.textMuted
        font: apx.font_narrow(theme.fontSmall)
    }
}
