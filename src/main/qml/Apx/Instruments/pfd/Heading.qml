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
import QtQuick 2.2
import "../common"
import "."

Item {
    id: hdg_window

    readonly property int m_mode: mandala.cmd.proc.mode.value

    readonly property var f_yaw: mandala.est.att.yaw
    readonly property var f_bearing: mandala.est.pos.bearing
    readonly property var f_cmd_bearing: mandala.cmd.pos.bearing
    //readonly property var f_thdg: mandala.est.wpt.thdg
    readonly property var f_adj: mandala.cmd.proc.adj

    readonly property var f_nomag: mandala.cmd.ahrs.nomag
    readonly property var f_rud: mandala.ctr.att.rud

    readonly property var f_ahrs_mag: mandala.est.ahrs.mag

    readonly property int m_reg_pos: mandala.cmd.reg.pos.value
    readonly property bool m_reg_taxi: mandala.cmd.reg.taxi.value
    property bool isTrack: m_reg_taxi || m_reg_pos===reg_pos_track || m_reg_pos===reg_pos_loiter

    readonly property bool isShiftControl: isTrack


    readonly property bool nomag: f_nomag.value > 0 || f_ahrs_mag.value === ahrs_mag_blocked


    //instrument item
    property double animation_duration: 500
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.leftMargin: 2
    anchors.rightMargin: 2
    height: width*0.09
    property double bottomHeight: height*0.4
    clip: true
    property double value: apx.angle(f_yaw.value)
    Behavior on value { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }

    property double num2scaleWidth: svgRenderer.elementBounds(pfdImageUrl, "hdg-scale").width * strip_scale /90
    property double strip_scale: width/svgRenderer.elementBounds(pfdImageUrl, "hdg-scale").width
    property double valueShiftMax: width/num2scaleWidth/2

    Item {
        id: scale_top
        anchors.fill: parent
        anchors.bottomMargin: parent.height*0.2
        clip: true

        //horizontal sacle with numbers
        Item {
            id: hdg_scale
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: -apx.angle(value)*num2scaleWidth
            width: parent.width*4*2
            height: parent.height

            //scale
            Repeater {
                model: 4*2
                PfdImage {
                    //smooth: ui.antialiasing
                    elementName: "hdg-scale"
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: index*width-tickWidth*strip_scale/2
                    width: elementBounds.width*strip_scale
                    height: elementBounds.height*strip_scale
                    property double tickWidth: 2
                }
            }
            //scale numbers
            Repeater {
                model: 36*2
                Text {
                    property int pos: (index-18*2)*10
                    property int num: apx.angle360(pos).toFixed()
                    //smooth: ui.antialiasing
                    text: num===0?qsTr("N"):
                          num===90?qsTr("E"):
                          num===180?qsTr("S"):
                          num===270?qsTr("W"):
                          ("00"+apx.angle360(pos).toFixed()).slice(-3)
                    //render as image
                    style: Text.Raised
                    styleColor: "transparent"
                    //color: "#A0FFFFFF"
                    color:(num===0||num===90||num===180||num===270)?"#B0FFFFFF":
                          "#80FFFFFF"
                    //color: "white"
                    font: apx.font_narrow(parent.height*0.6,true)
                    //anchors.fill: parent
                    anchors.top: parent.top
                    //anchors.right: parent.right
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: pos*num2scaleWidth
                    //anchors.topMargin: (index-2)*speed_window.num2scaleHeight
                }
            }
        }
        //gps bearing bug triangle
        PfdImage {
            id: hdg_crs_bug
            elementName: "hdg-crs-bug"
            property double value: apx.angle(f_bearing.value-f_yaw.value)
            Behavior on value { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            //smooth: ui.antialiasing
            border: 1
            fillMode: Image.PreserveAspectFit
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: apx.limit(apx.angle(value),-valueShiftMax,valueShiftMax)*num2scaleWidth
            height: bottomHeight
            width: elementBounds.width*height/elementBounds.height
            ToolTipArea { text: f_bearing.descr }
        }
        //cmd bearing bug arrow
        PfdImage {
            id: hdg_cmd_bug
            elementName: "hdg-cmd-bug"
            property double value: apx.angle(f_cmd_bearing.value-f_yaw.value)
            Behavior on value { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            //smooth: ui.antialiasing
            border: 1
            fillMode: Image.PreserveAspectFit
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: apx.limit(apx.angle(value),-valueShiftMax,valueShiftMax)*num2scaleWidth
            height: bottomHeight
            width: elementBounds.width*height/elementBounds.height
            ToolTipArea { text: f_cmd_bearing.descr }
        }
        //rw hdg bug arrow
        /*PfdImage {
            id: hdg_rw_bug
            visible: isTrack
            elementName: "hdg-rw-bug"
            property double value: apx.angle(f_thdg.value-f_yaw.value)
            Behavior on value { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            //smooth: ui.antialiasing
            border: 1
            fillMode: Image.PreserveAspectFit
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: apx.limit(apx.angle(value),-valueShiftMax,valueShiftMax)*num2scaleWidth
            height: bottomHeight
            width: elementBounds.width*height/elementBounds.height
            ToolTipArea { text: f_thdg.descr }
        }*/
        //center number box
        PfdImage {
            id: hdg_box
            elementName: "hdg-box"
            //smooth: ui.antialiasing
            fillMode: Image.PreserveAspectFit
            anchors.topMargin: 1
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height
            width: elementBounds.width*height/elementBounds.height
            Rectangle {
                border.width: 0
                color: "#C0FF0000"
                anchors.fill: hdg_text
                anchors.leftMargin: parent.width*0.05
                anchors.rightMargin: anchors.leftMargin
                anchors.topMargin: anchors.leftMargin+1
                anchors.bottomMargin: parent.height*0.4
                visible: nomag
            }
            Text {
                id: hdg_text
                anchors.fill: parent
                anchors.topMargin: -1
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignTop
                property int v: value
                text: ("00"+apx.angle360(v).toFixed()).slice(-3)
                font: apx.font_narrow(parent.height*0.7,true)
                color: nomag?"yellow":"white"

            }
            ToolTipArea { text: f_yaw.descr }
        }

    }

    //turn rate bar
    PfdImage {
        id: hdg_turnrate
        elementName: "hdg-turnrate"
        //smooth: ui.antialiasing
        fillMode: Image.PreserveAspectFit
        anchors.fill: parent
        anchors.topMargin: scale_top.height
        property double maxW: 0.95*elementBounds.width*height/elementBounds.height/2
        property double valueW: apx.limit(turn_calc.derivative*num2scaleWidth,-hdg_turnrate.maxW,hdg_turnrate.maxW)
        Behavior on valueW { enabled: ui.smooth; PropertyAnimation {duration: 500; } }
        //derivative
        Item {
            id: turn_calc
            visible: false
            property double value: f_yaw.value
            property double derivative: 0
            property double time_s: 0
            property double value_s: 0
            onValueChanged: {
                var t=new Date().getTime();
                var dt=(t-time_s)/1000;
                time_s=t;
                var dv=apx.angle(value-value_s);
                value_s=value;
                if(dt>0.5 || dt<0.01)derivative=0;
                else if(Math.abs(dv>50))derivative=0;
                else derivative=derivative*0.5+(dv/dt)*0.5;
            }
        }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.horizontalCenter
            anchors.topMargin: parent.height*0.15
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            border.color: "white"
            color: "white" //"#C0FF00FF"
            visible: hdg_turnrate.valueW>0
            width: hdg_turnrate.valueW
        }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.horizontalCenter
            anchors.topMargin: parent.height*0.15
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            border.color: "white"
            color: "white" //"#C0FF00FF"
            visible: hdg_turnrate.valueW<0
            width: -hdg_turnrate.valueW
        }
        //steering yaw control
        property double valueR: apx.limit(f_rud.value*hdg_turnrate.maxW,-hdg_turnrate.maxW,hdg_turnrate.maxW)
        Behavior on valueR { enabled: ui.smooth; PropertyAnimation {duration: 100; } }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.horizontalCenter
            anchors.topMargin: parent.height*0.25
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            color: "#80000000"
            visible: hdg_turnrate.valueR>0
            width: hdg_turnrate.valueR
        }
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.horizontalCenter
            anchors.topMargin: parent.height*0.25
            anchors.bottomMargin: anchors.topMargin
            border.width: 0
            antialiasing: true
            color: "#80000000"
            visible: hdg_turnrate.valueR<0
            width: -hdg_turnrate.valueR
        }

    }

    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: parent.width/2
        cursorShape: Qt.PointingHandCursor
        //propagateComposedEvents: true
        onClicked: {
            if(isShiftControl) f_adj.setValue(f_adj.value-1)
            else f_cmd_bearing.setValue(apx.angle(f_cmd_bearing.value-15))
        }
    }
    MouseArea {
        anchors.fill: parent
        anchors.leftMargin: parent.width/2
        cursorShape: Qt.PointingHandCursor
        //propagateComposedEvents: true
        onClicked: {
            if(isShiftControl) f_adj.setValue(f_adj.value+1)
            else f_cmd_bearing.setValue(apx.angle(f_cmd_bearing.value+15))
        }
    }

    StatusFlag {
        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -width
        height: pfdScene.flagHeight
        fact: f_ahrs_mag
        text: qsTr("MAG")
        status_warning: ahrs_mag_warning
        status_reset: ahrs_mag_unknown
        //status_show: ahrs_mag_blocked
    }
    CleanText {
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: height
        height: pfdScene.flagHeight
        fact: f_ahrs_mag
        type: CleanText.Clean
        show: fact.value > ahrs_mag_3D
    }

}
