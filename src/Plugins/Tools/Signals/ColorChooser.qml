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
import QtQuick.Controls

import Apx.Common
import APX.Facts

Item {
    id: colorChooser

    property var color: fact.value !== undefined ? fact.value : "#ffffff"
    property var space: Style.spacing * 1.2

    implicitWidth: 280 * ui.scale
    implicitHeight: 100 * ui.scale

    Rectangle {
        id: colorBox
        border.width: 0
        color: "#282828"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: colorPreview.top

        GridLayout {
            columns: 12
            anchors.fill: parent
            anchors.margins: space
            columnSpacing: space
            rowSpacing: space

            Repeater {
                model: [
                    "#ff8a8a", "#ffc0cb", "#eee8aa", "#ffffe0", "#98fb98", "#acbf69", "#add8e6", "#4169e1", "#cf9fff", "#d8bfd8", "#ffebcd", "#ffffff",
                    "#ff7f50", "#ff69b4", "#ffd580", "#ffff8f", "#00ff00", "#6b8e23", "#87ceeb", "#0000ff", "#da70d6", "#dda0dd", "#deb887", "#d3d3d3",
                    "#ff4500", "#ff1493", "#ffa500", "#ffff00", "#32cd32", "#556b2f", "#00bfff", "#0000cd", "#bf40bf", "#ee82ee", "#d2691e", "#808080",
                    "#ff0000", "#dc143c", "#ff8c00", "#ffd700", "#008000", "#3c4d03", "#1e90ff", "#000080", "#800080", "#ba55d3", "#a52a2a", "#000000"
                ]

                delegate: Rectangle {
                    property bool chosen: mouseArea.containsMouse
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: modelData
                    border.width: 1
                    border.color: chosen ? "#b0c4de" : "transparent"
                    opacity: chosen ? 1 : 0.85
                    scale: chosen ? 1.15 : 1.0

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: fact.value = modelData
                    }
                }
            }
        }
    }
    RowLayout {
        id: colorPreview
        anchors.bottom: parent.bottom
        spacing: space

        Rectangle {
            color: colorChooser.color
            width: 24 * ui.scale
            height: 16 * ui.scale
            border.width: ui.scale
            border.color: "#383838"
            Layout.topMargin: space
            Layout.leftMargin: space
            opacity: 0.85
        }

        Label {
            text: colorChooser.color
            font.pixelSize: Style.fontSize * 0.7
            Layout.topMargin: space
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
