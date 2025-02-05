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
//import QtQuick 2.2
//import QtQuick.Controls 1.1
import "../common"

ControlArea {

    readonly property var f_altitude: mandala.fact("est.pos.altitude")
    readonly property var f_cmd_altitude: mandala.fact("cmd.pos.altitude")

    readonly property var f_ins_dh: mandala.fact("est.ins.dh")

    readonly property var f_agl: mandala.fact("est.pos.agl")
    readonly property bool m_ins_hagl: mandala.fact("cmd.ins.hagl").value
    readonly property bool m_agl_status: mandala.fact("sns.agl.status").value

    readonly property bool m_agl_valid: m_ins_hagl



    mvar: f_cmd_altitude   //ControlArea
    span: 20
    min:0
    max: 50000
    fixedPoint: true
    step: (f_cmd_altitude.value<100)?1:f_cmd_altitude.value<800?10:100
    stepDrag: step*2
    stepWheel: step*0.0001
    stepLimit: 100

    //instrument item
    property double anumation_duration: 200
    anchors.fill: parent

    Rectangle {
        id: altitude_window
        color: "transparent"
        border.width: 0
        clip: true
        anchors.fill: parent

        property double strip_width: 0.2
        property double strip_factor: 10

        property real altitude_value : f_altitude.value
        Behavior on altitude_value { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }

        property variant scale_bounds: svgRenderer.elementBounds(pfdImageUrl, "altitude-scale")
        property double strip_scale: (width>0?width:10)*strip_width/scale_bounds.width
        property double num2scaleHeight: scale_bounds.height * strip_scale /10

        Item {
            id: altitude_scale
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 1
            anchors.left: parent.left
            width: parent.width*altitude_window.strip_width

            property int item_cnt: Math.floor(altitude_window.height/altitude_window.num2scaleHeight*10/2)*2+2
            Repeater {
                model: 8    //altitude_scale.item_cnt
                PfdImage {
                    id: altitude_scale_image
                    elementName: "altitude-scale"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 1
                    anchors.right: parent.right
                    anchors.verticalCenterOffset: (index-2)*height + height/10 * (altitude_window.altitude_value/altitude_window.strip_factor-Math.floor(altitude_window.altitude_value/altitude_window.strip_factor))
                    sourceScale: altitude_window.strip_scale
                }
            }
        }

        Item {
            id: scale_numbers

            anchors.left: parent.left
            anchors.leftMargin: altitude_window.width*altitude_window.strip_width
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            property int item_cnt: Math.floor(altitude_window.height/altitude_window.num2scaleHeight/2)*2+2
            property int topNumber: Math.floor(altitude_window.altitude_value/altitude_window.strip_factor)+item_cnt/2
            Repeater {
                model: scale_numbers.item_cnt
                Item {
                    width: scale_numbers.width
                    transform: [
                        Scale { yScale: 1.01 }, //force render as img
                        Translate { y: (index-scale_numbers.item_cnt/2+(altitude_window.altitude_value/altitude_window.strip_factor-Math.floor(altitude_window.altitude_value/altitude_window.strip_factor)))*altitude_window.num2scaleHeight }
                    ]

                    Text {
                        property int num: scale_numbers.topNumber - index
                        //smooth: ui.antialiasing
                        text: num*altitude_window.strip_factor
                        visible: num>=0
                        //render as image
                        style: Text.Raised
                        styleColor: "transparent"

                        color: "#A0FFFFFF"
                        font: apx.font_narrow(altitude_window.num2scaleHeight / 1.5,true)
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        PfdImage {
            id: altitude_waypoint
            elementName: "altitude-waypoint"
            sourceScale: altitude_window.strip_scale
            width: elementBounds.width*height/elementBounds.height
            height: altitude_box.height

            anchors.left: altitude_scale.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: apx.limit(altitude_window.num2scaleHeight * (f_altitude.value - f_cmd_altitude.value)/altitude_window.strip_factor,-parent.height/2,parent.height/2)
            Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
            ToolTipArea {text: f_cmd_altitude.descr}
        }

        PfdImage {
            id: altitude_box
            elementName: "altitude-box"
            clip: true
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width: parent.width
            height: elementBounds.height*width/elementBounds.width
            anchors.leftMargin: 5

            StripNum {
                anchors.fill: parent
                anchors.topMargin: parent.height*0.06
                anchors.bottomMargin: anchors.topMargin
                anchors.leftMargin: parent.width*0.85
                anchors.rightMargin: parent.width*0.03
                //numScale: 0.35
                numGapScale: 1.2 //0.9
                value: altitude_window.altitude_value
            }
            Row {
                spacing: 0
                layoutDirection: Qt.RightToLeft
                anchors.fill: parent
                anchors.topMargin: parent.height*0.25
                anchors.bottomMargin: parent.height*0.2
                anchors.leftMargin: parent.width*0.1
                anchors.rightMargin: parent.width*0.15
                Repeater {
                    model: 4
                    StripNum {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width*0.2
                        numScale: 1.2
                        color: (value<divider)?"gray":"white"
                        divider: Math.pow(10,index+1)
                        value: altitude_window.altitude_value
                    }
                }
            }
            ToolTipArea {text: f_altitude.descr}
        }

    }

    PfdImage {
        id: altitude_triangle

        readonly property real v: f_ins_dh.value

        elementName: "altitude-triangle"
        visible: ui.test || apx.datalink.valid

        sourceScale: altitude_window.strip_scale
        anchors.right: altitude_window.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: apx.limit(altitude_window.num2scaleHeight * v/altitude_window.strip_factor,-altitude_window.height/2,altitude_window.height/2)
        Behavior on anchors.verticalCenterOffset { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }

        Text {
            visible: Math.abs(altitude_triangle.v)>5
            text: altitude_triangle.v>0?("+"+altitude_triangle.v.toFixed()):altitude_triangle.v.toFixed()
            color: "white"
            anchors.right: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 4
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: apx.font_narrow(parent.height)
        }
        ToolTipArea {text: f_ins_dh.descr}
    }

    PfdImage {
        id: agl_image
        elementName: "agl"
        visible: m_agl_valid
        sourceScale: altitude_window.strip_scale
        anchors.right: altitude_window.left
        anchors.top: parent.verticalCenter
        anchors.topMargin: apx.limit(5*altitude_window.num2scaleHeight * (f_agl.value)/altitude_window.strip_factor,-altitude_window.height/2,altitude_window.height/2)
        Behavior on anchors.topMargin { enabled: ui.smooth; PropertyAnimation {duration: anumation_duration} }
        Text {
            visible: f_agl.value>0
            text: f_agl.value.toFixed(f_agl.value>10?0:1)
            color: m_agl_status?"yellow":"red"
            anchors.right: parent.left
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: apx.font_narrow(parent.height)
        }
        ToolTipArea {text: f_agl.descr}
    }

}
