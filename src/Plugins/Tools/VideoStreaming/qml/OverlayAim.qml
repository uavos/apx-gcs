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
import QtGraphicalEffects

Item {
    id: control

    property int w: 2
    property int size: 100

    width: size
    height: size

    enum AimType {
        None,
        Crosshair,
        Rectangle
    }

    property int type: OverlayAim.AimType.Crosshair

    Item {
        id: content
        anchors.fill: parent
        visible: false
        Loader {
            active: control.type === OverlayAim.AimType.Crosshair
            anchors.fill: parent
            sourceComponent: Item {
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: control.w
                    height: parent.height
                    border.width: 0
                    color: "#fff"
                }
            }
        }
        Loader {
            active: control.type === OverlayAim.AimType.Rectangle
            anchors.fill: parent
            sourceComponent: Item {
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
            }
        }
    }
    DropShadow {
        anchors.fill: content
        samples: 15
        color: "#000"
        source: content
        cached: true
    }
}
