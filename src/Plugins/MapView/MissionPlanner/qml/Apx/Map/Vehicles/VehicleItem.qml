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
import QtLocation 5.12
import QtPositioning 5.12

import QtGraphicalEffects 1.0

import Apx.Common 1.0

import APX.Vehicles 1.0 as APX


MapQuickItem {  //to be used inside MapComponent only
    id: vehicleItem

    readonly property APX.Vehicle vehicle: modelData

    //Fact bindings
    readonly property var vm: vehicle.mandala
    readonly property real f_roll: vm.est.att.roll.value
    readonly property real f_pitch: vm.est.att.pitch.value
    readonly property real f_yaw: vm.est.att.yaw.value
    readonly property real f_altitude: vm.est.pos.altitude.value
    readonly property real f_cmd_bearing: vm.cmd.pos.bearing.value
    readonly property real f_bearing: vm.est.pos.bearing.value
    readonly property real f_windHdg: vm.est.wind.heading.value
    readonly property real f_windSpd: vm.est.wind.speed.value
    readonly property int f_mode: vm.cmd.proc.mode.value

    readonly property bool f_LDTO: f_mode === proc_mode_LANDING || f_mode === proc_mode_TAKEOFF

    readonly property real m_xtrack: mandala.est.wpt.xtrack.value

    readonly property int m_reg_pos: mandala.cmd.reg.pos.value
    readonly property bool m_reg_taxi: mandala.cmd.reg.taxi.value
    property bool isTrack: m_reg_taxi || m_reg_pos===reg_pos_track || m_reg_pos===reg_pos_loiter

    readonly property bool active: vehicle.active

    visible: vehicle.visible

    readonly property bool bGCU: vehicle.isGroundControl
    readonly property bool bLOCAL: vehicle.isLocal


    Connections {
        target: vehicle
        function onTriggered() {
            if(active) focusTimer.start()
        }
    }

    //position (smooth)
    property var coord: QtPositioning.coordinate(vehicle.coordinate.latitude,vehicle.coordinate.longitude,f_altitude)
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
        target: vehicleItem
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
            if(vehicle.coordinate.latitude!==0 && vehicle.coordinate.longitude!==0){
                focusTimer.stop()
                map.showCoordinate(vehicleItem.coordinate)
            }
        }
    }
    readonly property bool mapFollow: map.itemToFollow===vehicleItem && map.follow
    readonly property bool follow: vehicle.follow
    onFollowChanged: {
        map.followStop()
        if(follow)map.followItem(vehicleItem)
    }
    onMapFollowChanged: {
        if(!mapFollow)vehicle.follow=false
    }

    //animated vars
    property real vyaw: f_yaw
    Behavior on vyaw { enabled: ui.smooth; RotationAnimation {duration: animation_duration; direction: RotationAnimation.Shortest; } }


    //info box
    property bool bInfoShowRight: true
    property bool bInfoPosChk2: f_yaw<=50 || f_yaw>120
    property bool bInfoPosChk1: f_yaw<=40 || f_yaw>130
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
    property int animation_duration: 100

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
            visible: active
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
            visible: active
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
            visible: active && isTrack
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
            color: "#fff" //"#fd6"
            source: "../icons/wind-arrow.svg"
            sourceSize.height: image.width
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
        Rectangle {
            anchors.centerIn: image
            VehicleLabel {
                id: label
                z: image.z-5
                vehicle: vehicleItem.vehicle
                colorFG: "#fff"
                showDots: false
                anchors.centerIn: parent
                anchors.verticalCenterOffset: bInfoShowRight?0:(image.height/2+height/2)
                anchors.horizontalCenterOffset: bInfoShowRight?(image.width/2+width/2):0
            }
            DropShadow {
                anchors.fill: label
                horizontalOffset: 3
                verticalOffset: 3
                radius: 8.0
                samples: 17
                color: "#c0000000"
                source: label
            }
        }

        MouseArea {
            anchors.fill: image
            drag.target: vehicleItem
            onClicked: {
                if(!active) apx.vehicles.selectVehicle(vehicle);
                else vehicle.follow=!vehicle.follow
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
