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
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.13

import Apx.Common 1.0

Item {
    id: frame
    property var plugin

    state: plugin.state

    states: [
        State {
            name: "maximized"
            PropertyChanges {
                target: frame
                visible: false
            }
        },
        State {
            name: "minimized"
            PropertyChanges {
                target: frame
                visible: plugin.active
                implicitWidth: Style.buttonSize*8
                implicitHeight: implicitWidth*3/4
            }
            PropertyChanges {
                target: plugin
                parent: content
                anchors.fill: content
            }
        }
    ]

    Component.onCompleted: {
        plugin.parent=content
        plugin.anchors.fill=content
    }

    readonly property bool minimized: state!="maximized"
    visible: plugin.active

    implicitWidth: plugin.implicitWidth
    implicitHeight: plugin.implicitHeight
    DropShadow {
        enabled: minimized && ui.effects>1
        anchors.fill: parent
        source: content.background
        samples: enabled?15:0
        color: "#000"
        cached: true
        radius: samples/2
    }
    Item {
        id: content
        anchors.fill: parent
        property alias background: bgRect
        Rectangle {
            id: bgRect
            anchors.fill: parent
            border.width: 0
            color: "#000"
            radius: minimized?5:0
        }
        layer.enabled: minimized && ui.effects>0
        layer.effect: OpacityMask { maskSource: bgRect }
        Loader {
            z: 9999
            anchors.left: parent.left
            anchors.top: parent.top
            active: plugin.state==="minimized"
            sourceComponent: IconButton {
                color: "transparent"
                iconName: "fullscreen"
                toolTip: qsTr("Maximize view")
                onTriggered: plugin.state="maximized"
            }
        }
    }
}
