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

    readonly property int m_mode: mandala.cmd.proc.mode.value
    readonly property int m_stage: mandala.cmd.proc.stage.value
    readonly property int m_reg_airbrk: mandala.cmd.reg.airbrk.value

    readonly property var f_derr: mandala.est.wpt.derr
    readonly property var f_xtrack: mandala.est.wpt.xtrack

    readonly property int m_reg_hdg: mandala.cmd.reg.hdg.value
    readonly property bool m_reg_taxi: mandala.cmd.reg.taxi.value
    property bool isTrack: m_reg_taxi || m_reg_hdg===reg_hdg_track || m_reg_hdg===reg_hdg_loiter
        || (m_mode==proc_mode_UAV && m_stage>0)

    property double anumation_duration: 1000

    readonly property bool isLanding: m_mode===proc_mode_LANDING
    readonly property bool isDistTracking: m_reg_airbrk===reg_airbrk_dist 
        || (m_mode==proc_mode_UAV && m_stage>0)

    property double sz: (width>height?height:width)*0.6
    
    PfdImage {
        id: ils_bar_vertical
        visible: ui.test || isDistTracking
        elementName: "ils-bar-vertical"
        fillMode: Image.PreserveAspectFit
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: sz
        width: elementBounds.width*height/elementBounds.height
        //smooth: ui.antialiasing
    }
    
    Rectangle {
        visible: ui.test || isLanding || isDistTracking
        antialiasing: true
        color: "#3f3"
        border.width: 0.5
        border.color: "#80000000"
        width: ils_bar_vertical.width*1.5
        height: ils_bar_vertical.width*0.5
        anchors.centerIn: ils_bar_vertical
        anchors.verticalCenterOffset: apx.limit(f_derr.value/500*ils_bar_vertical.height/2,-ils_bar_vertical.height*0.6,ils_bar_vertical.height*0.6)
        Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration; } }
        Text {
            property double value: Math.abs(f_derr.value.toFixed())
            visible: value>25
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.left
            text: value
            color: "white"
            font: apx.font_narrow(parent.width)
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
