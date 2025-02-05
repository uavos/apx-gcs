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
import QtQuick.Effects
import QtLocation
import QtPositioning


import Apx.Common

import APX.Fleet as APX


MapQuickItem {  //to be used inside MapComponent only
    id: unitItem

    readonly property APX.Unit unit: modelData

    //Fact bindings
    readonly property var vm: unit.mandala
    readonly property real f_roll: vm.fact("est.att.roll").value
    readonly property real f_pitch: vm.fact("est.att.pitch").value
    readonly property real f_yaw: vm.fact("est.att.yaw").value
    readonly property real f_altitude: vm.fact("est.pos.altitude").value
    readonly property real f_cmd_bearing: vm.fact("cmd.pos.bearing").value
    readonly property real f_bearing: vm.fact("est.pos.bearing").value
    readonly property real f_windHdg: vm.fact("est.wind.heading").value
    readonly property real f_windSpd: vm.fact("est.wind.speed").value
    readonly property int f_mode: vm.fact("cmd.proc.mode").value

    readonly property bool f_LDTO: f_mode == vm.proc_mode_LANDING || f_mode == vm.proc_mode_TAKEOFF

    readonly property real m_xtrack: vm.fact("est.wpt.xtrack").value

    readonly property int m_reg_hdg: vm.fact("cmd.reg.hdg").value
    readonly property bool m_reg_taxi: vm.fact("cmd.reg.taxi").value
    property bool isTrack: m_reg_taxi || m_reg_hdg == vm.reg_hdg_track || m_reg_hdg == vm.reg_hdg_loiter

    readonly property bool active: unit.active

    visible: unit.visible

    readonly property bool bGCU: unit.isGroundControl
    readonly property bool bLOCAL: unit.isLocal

    readonly property bool bXPDR: unit.streamType===APX.PUnit.XPDR


    Connections {
        target: unit
        function onTriggered() {
            if(active) focusTimer.start()
        }
    }

    //position (smooth)
    property var coord: QtPositioning.coordinate(unit.coordinate.latitude,unit.coordinate.longitude,f_altitude)
    onCoordChanged: {
        if(map.isFarMove(coordinate,coord,2)){
            anim.stop()
            coordinate=coord
            return
        }
        if(!anim.running)anim.from=coordinate
        anim.to=coord
        anim.start()
    }
    CoordinateAnimation {
        id: anim
        duration: ui.smooth?500:0
        direction: CoordinateAnimation.Shortest
        easing.type: Easing.Linear
        target: unitItem
        property: "coordinate"
    }

    //follow
    onActiveChanged: {
        if(active) focusTimer.start()
    }
    Timer {
        id: focusTimer
        interval: 100
        repeat: true
        running: false
        onTriggered: {
            if(unit.coordinate.latitude!==0 && unit.coordinate.longitude!==0){
                focusTimer.stop()
                map.showCoordinate(unitItem.coordinate)
            }
        }
    }
    readonly property bool mapFollow: map.itemToFollow===unitItem && map.follow
    readonly property bool follow: unit.follow
    onFollowChanged: {
        map.followStop()
        if(follow)map.followItem(unitItem)
    }
    onMapFollowChanged: {
        if(!mapFollow)unit.follow=false
    }

    //animated vars
    property real vyaw: bXPDR?f_bearing:f_yaw
    Behavior on vyaw { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }


    //info box
    property bool bInfoShowRight: true
    property bool bInfoPosChk2: vyaw<=50 || vyaw>120
    property bool bInfoPosChk1: vyaw<=40 || vyaw>130
    onBInfoPosChk1Changed: {
        if(bGCU)return;
        if(bInfoShowRight)return;
        if(bInfoPosChk1)bInfoShowRight=true
    }
    onBInfoPosChk2Changed: {
        if(bGCU)return;
        if(!bInfoShowRight)return;
        if(!bInfoPosChk2)bInfoShowRight=false
    }


    //constants
    property int animation_duration: bXPDR?500:100

    anchorPoint.x: image.width/2
    anchorPoint.y: image.height/2

    sourceItem:
    Item {
        Image {
            id: image
            source: bGCU?"../icons/gcu.svg":"../icons/uav.svg"
            property int sz: (bGCU?32:48)*map.itemsScaleFactor*ui.scale
            sourceSize.height: sz
            sourceSize.width: sz

            opacity: ui.effects?(active?0.9:0.7):1
            scale: active?1:0.6

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: ui.effects
            }

            transform: [
                Scale {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    xScale: 1/(0.05*map.itemsScaleFactor);
                    yScale: xScale;
                },
                Rotation {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    axis.y: 1
                    axis.z: 0
                    readonly property real maxA: 60
                    angle: (f_roll>maxA?maxA:(f_roll<-maxA?-maxA:f_roll))
                    Behavior on angle { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
                },
                Rotation {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    axis.x: 1
                    axis.z: 0
                    readonly property real maxA: 60
                    angle: -(f_pitch>maxA?maxA:(f_pitch<-maxA?-maxA:f_pitch))
                    Behavior on angle { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
                },
                Scale {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    xScale: 0.05*map.itemsScaleFactor;
                    yScale: xScale;
                },
                Rotation {
                    origin.x: image.width/2
                    origin.y: image.height/2
                    axis.z: 1
                    angle: vyaw-map.bearing
                }
            ]
        }

        Image {
            id: cmdCrsArrow
            source: "../icons/cmd-crs-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            fillMode: Image.PreserveAspectFit
            //smooth: ui.antialiasing
            visible: active && !bXPDR
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: cmdCrsArrow.width/2
                origin.y: cmdCrsArrow.height
                property real v: f_cmd_bearing
                angle: v-map.bearing
                Behavior on v { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }
        }
        Image {
            id: crsArrow
            source: "../icons/crs-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            fillMode: Image.PreserveAspectFit
            //smooth: ui.antialiasing
            visible: active
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: crsArrow.width/2
                origin.y: crsArrow.height
                property real v: f_bearing
                angle: v-map.bearing
                Behavior on v { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }
            }
        }
        Image {
            id: hdgArrow
            source: "../icons/hdg-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            fillMode: Image.PreserveAspectFit
            //smooth: ui.antialiasing
            visible: active && !bXPDR
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: crsArrow.width/2
                origin.y: crsArrow.height
                angle: vyaw-map.bearing
            }
        }
        Image {
            id: xtrackArrow
            source: "../icons/hdg-arrow.svg"
            z: image.z-10
            sourceSize.height: image.width*2
            fillMode: Image.PreserveAspectFit
            visible: active && isTrack && !bXPDR
            anchors.bottom: image.verticalCenter
            anchors.horizontalCenter: image.horizontalCenter
            readonly property real max: image.width*0.8
            readonly property real value: limit(-m_xtrack,-max,max)
            transform: [
                Translate{
                    x: xtrackArrow.value
                },
                Rotation{
                    origin.x: xtrackArrow.width/2
                    origin.y: xtrackArrow.height
                    angle: vyaw-map.bearing
                }
            ]
        }

        SvgImage {
            id: windArrow
            z: image.z-100
            visible: active && (f_LDTO||follow) && f_windSpd>0
            source: "../Map/icons/wind-arrow.svg"
            size: image.width
            anchors.top: image.verticalCenter
            anchors.topMargin: image.width*1.5
            anchors.horizontalCenter: image.horizontalCenter
            transform: Rotation{
                origin.x: windArrow.width/2
                origin.y: -windArrow.anchors.topMargin
                angle: windArrow.v-map.bearing
            }
            opacity: ui.effects?0.9:1
            property real v: f_windHdg
            Behavior on v { enabled: ui.smooth; RotationAnimation {duration: 1000; direction: RotationAnimation.Shortest; } }
        }


        //info label
        Item {
            anchors.centerIn: image
            UnitLabel {
                id: label
                z: image.z-5
                unit: unitItem.unit
                colorFG: "#fff"
                colorBG: "#aaa"
                showDots: false
                anchors.centerIn: parent
                anchors.verticalCenterOffset: bInfoShowRight?0:(image.height/2+height/2)
                anchors.horizontalCenterOffset: bInfoShowRight?(image.width/2+width/2):0
            }
            MultiEffect {
                source: label
                anchors.fill: label
                shadowEnabled: true
            }
        }

        MouseArea {
            anchors.fill: image
            drag.target: unitItem
            onClicked: {
                if(!active) apx.fleet.selectUnit(unit);
                else unit.follow=!unit.follow
            }
        }

        transform: [
            Rotation {
                origin.x: image.width/2
                origin.y: image.height/2
                axis.x: 1
                axis.z: 0
                angle: map.tilt
            }
        ]
    }
}
