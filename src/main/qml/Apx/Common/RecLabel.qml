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
import QtQuick 2.5;
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3

import APX.Vehicles 1.0 as APX

import Apx.Common 1.0

Item {
    id: control
    implicitWidth: textItem.implicitWidth+5+5

    readonly property APX.Vehicle vehicle: apx.vehicles.current
    readonly property var fact: vehicle.telemetry

    readonly property alias color: textItem.color
    readonly property bool active: fact.active
    readonly property bool replay: vehicle.isReplay

    property bool hovered: mouseArea.containsMouse

    opacity: ui.effects?(hovered?0.8:(active||replay)?1:0.5):1
    Behavior on opacity { enabled: ui.smooth; NumberAnimation {duration: 100; } }

    Text {
        id: textItem
        anchors.fill: parent
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        font.family: font_narrow
        font.pixelSize: Math.max(8,control.height)
        verticalAlignment: Text.AlignVCenter
        color: ((!replay) && active && fact.time>=(4*60*60))?"#FF9800":"#fff"
        text: fact.text
    }
    Rectangle {
        anchors.fill: control;
        color: "transparent"
        border.width: 2
        radius: height/10
        border.color: active?"#8f8":replay?"#478fff":"#fff"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: fact.enabled && (!replay)
        hoverEnabled: enabled
        cursorShape: enabled?Qt.PointingHandCursor:Qt.ArrowCursor
        onClicked: fact.recorder.recording=!fact.recorder.recording
        ToolTip {
            delay: 1500
            timeout: 5000
            visible: mouseArea.containsMouse
            text: fact.descr
        }
    }

}
