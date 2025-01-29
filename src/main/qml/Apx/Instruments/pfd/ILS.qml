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
import "../common"

Item {
    id: ils_window

    readonly property int m_mode: mandala.fact("cmd.proc.mode").value

    readonly property var f_delta: mandala.fact("est.wpt.derr")
    readonly property var f_xtrack: mandala.fact("est.wpt.xtrack")

    readonly property int m_reg_hdg: mandala.fact("cmd.reg.hdg").value
    readonly property bool m_reg_taxi: mandala.fact("cmd.reg.taxi").value
    property bool isTrack: m_reg_taxi || m_reg_hdg===reg_hdg_track || m_reg_hdg===reg_hdg_loiter

    property double anumation_duration: 1000

    property bool isLanding: m_mode===proc_mode_LANDING

    property double sz: (width>height?height:width)*0.6

    PfdImage {
        id: ils_bar_vertical
        visible: ui.test || isLanding
        elementName: "ils-bar-vertical"
        fillMode: Image.PreserveAspectFit
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: sz
        width: elementBounds.width*height/elementBounds.height
        //smooth: ui.antialiasing
        Rectangle {
            antialiasing: true
            color: "#3f3"
            border.width: 0.5
            border.color: "#80000000"
            width: parent.width*1.5
            height: parent.width*0.5
            anchors.centerIn: parent
            anchors.verticalCenterOffset: apx.limit(f_delta.value/500*parent.height/2,-parent.height*0.6,parent.height*0.6)
            Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration; } }
            Text {
                property double value: Math.abs(f_delta.value.toFixed())
                visible: value>25
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.left
                text: value
                color: "white"
                font: apx.font_narrow(parent.width)
            }
        }
    }

    PfdImage {
        id: ils_bar_horizontal
        visible: isTrack
        elementName: "ils-bar-horizontal"
        fillMode: Image.PreserveAspectFit
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: ils_bar_vertical.height
        height: elementBounds.height*width/elementBounds.width
        //smooth: ui.antialiasing
        Rectangle {
            antialiasing: true
            color: "#3f3"
            border.width: 0.5
            border.color: "#80000000"
            width: parent.height*0.5
            height: parent.height*1.5
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: apx.limit(-f_xtrack.value/20*parent.width/2,-parent.width*0.6,parent.width*0.6)
            Behavior on anchors.horizontalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration; } }
            Text {
                property double value: Math.abs(f_xtrack.value.toFixed())
                visible: value>5 && value<100
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                text: value
                color: "white"
                font: apx.font_narrow(parent.height)
            }
        }
    }


}
