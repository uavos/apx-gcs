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
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12

import Apx.Common 1.0

Item {
    id: control

    property real value: mandala.est.att.roll.value //default sample

    property int w: Math.max(2,width*0.08)

    enum AxisType {
        Full,
        Down
    }

    property int type: OverlayGimbalAxis.AxisType.Down


    implicitWidth: 100
    implicitHeight: width


    opacity: 0.3
    onValueChanged: opacity=1


    readonly property real cx: width / 2
    readonly property real r: Math.max(1, width/2 - w*2)


    property real bias: 0
    property real start: 0
    property real end: 360
    property bool reverse: false

    Component.onCompleted: {
        switch(control.type){
        default:
        case OverlayGimbalAxis.AxisType.Full:
            bias = -90
            break;
        case OverlayGimbalAxis.AxisType.Down:
            start = -45
            end = 270-45
            reverse=true
            break;
        }
    }

    Canvas {
        id: content
        anchors.fill: parent
        visible: false
        contextType: "2d"
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.beginPath()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 1)
            ctx.lineWidth = control.w
            ctx.arc(control.cx, control.cx, control.r, control.start*Math.PI/180, control.end*Math.PI/180, false)
            ctx.stroke()
        }
        Rectangle {
            id: arrow
            border.width: 0
            color: Material.color(Material.Green)
            width: w*3
            height: w*1.5
            radius: height/5
            x: cx
            y: cx
            property real v: control.reverse?-control.value:control.value
            Behavior on v { RotationAnimation { duration: 200; direction: RotationAnimation.Shortest } }
            transform: [
                Translate { x: r-arrow.width/2; y: -arrow.height/2; },
                Rotation { origin.x: 0; origin.y: 0; angle: arrow.v+control.bias }
            ]
        }
    }
    DropShadow {
        anchors.fill: content
        samples: 15
        color: "#000"
        source: content
        cached: true
    }
    MaterialIcon {
        name: "chevron-double-right"
        size: control.r //*0.9
        anchors.centerIn: parent
        rotation: control.bias
    }
}
