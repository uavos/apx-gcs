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
import Apx.Application

// Terrain elevation under the cursor, shown in the info line of the map.
// The map only gives the place for it (ui.mapInfo.add), it knows nothing about
// the plugin. Hidden while the plugin is passive.
AppPlugin {
    id: plugin

    uiComponent: "mapInfo"
    visible: fact ? fact.active : false
    Layout.alignment: Qt.AlignVCenter
    onConfigure: ui.mapInfo.add(plugin)

    sourceComponent: Item {
        id: elevationItem
        readonly property real elevation: plugin.fact ? plugin.fact.elevation : NaN
        // no data under the cursor: dimmed icon, empty text, width is kept
        readonly property color color: isNaN(elevation) ? "#808080" : "#fff"

        implicitHeight: ui.mapInfo.size
        implicitWidth: Math.max(elevationIcon.width+elevationMetrics.width, height*4)

        TextMetrics {
            id: elevationMetrics
            font: elevationText.font
            text: "8888m"
        }
        MaterialIcon {
            id: elevationIcon
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            name: "elevation-rise"
            color: elevationItem.color
            size: height
        }
        Text {
            id: elevationText
            anchors.left: elevationIcon.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            verticalAlignment: Text.AlignVCenter
            font: apx.font_narrow(Style.fontSize)
            color: elevationItem.color
            text: isNaN(elevationItem.elevation) ? "" : Math.round(elevationItem.elevation) + "m"
        }
        ToolTipArea {
            text: qsTr("Point elevation above sea level")
            cursorShape: Qt.PointingHandCursor
            onClicked: plugin.fact.trigger() // plugin settings
        }
    }
}
