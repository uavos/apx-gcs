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
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtPositioning

import Apx.Common

import APX.Vehicles as APX


Item {
    id: control

    property real fontSize: Style.fontSize
    property real fontSizeInfo: fontSize*0.8
    readonly property real dotSize: fontSize/2

    property APX.Vehicle vehicle

    property bool showDots: true
    property bool showInfo: true

    property color colorFG: vehicle.active?"#fff":"#aaa"
    property color colorBG: {
        var c
        if(bGCU) c="#3779C5"
        else if(vehicle.streamType===APX.PVehicle.TELEMETRY)c="#377964"
        else if(vehicle.streamType===APX.PVehicle.XPDR)c="#376479"
        else c="#555" // offline
        if(!vehicle.active)c=Qt.darker(c,1.9)
        return c
    }

    //Fact bindings
    readonly property bool bGCU: vehicle.isGroundControl
    readonly property bool bLOCAL: vehicle.isLocal


    //internal
    property string callsign: vehicle.title


    property real paddingRight: dotSize+3

    implicitWidth: textLayout.width+paddingRight
    implicitHeight: textLayout.height

    Connections {
        target: textLayout
        function onImplicitWidthChanged(){ timerWidthUpdate.start() }
    }
    property Timer timerWidthUpdate: Timer {
        interval: 1
        onTriggered: {
            implicitWidth=Math.max(textLayout.implicitWidth+paddingRight,implicitWidth)
        }
    }

    //right side info
    ColumnLayout {
        visible: showDots
        spacing: Style.spacing
        anchors.fill: parent

        //recording red point
        Rectangle {
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.topMargin: Style.spacing
            visible: vehicle.telemetry.active
            border.width: 0
            implicitWidth: dotSize
            implicitHeight: dotSize
            radius: height/2
            color: "#C0FF8080"
        }
        //mission available
        Rectangle {
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.topMargin: Style.spacing
            visible: vehicle && vehicle.mission.missionSize>0
            border.width: 0
            implicitWidth: dotSize
            implicitHeight: dotSize
            radius: height/2
            color: "#C080FFFF"
        }
        Item {
            Layout.fillHeight: true
        }
    }

    ColumnLayout {
        id: textLayout
        spacing: 0
        Text {
            Layout.minimumWidth: control.fontSize
            horizontalAlignment: Text.AlignLeft
            font: apx.font(control.fontSize,true)
            text: callsign
            color: colorFG
        }
        Text {
            id: infoText
            Layout.fillHeight: true
            Layout.minimumWidth: control.fontSizeInfo
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            font: apx.font_narrow(control.fontSizeInfo)
            lineHeight: 0.75
            text: vehicle.info
            color: colorFG

            visible: showInfo && !bLOCAL

            // make sure implicit width always increase, never decrease
            onImplicitWidthChanged: {
                if(vehicle.isIdentified)
                    timerWidthUpdate.start()
            }
            property Timer timerWidthUpdate: Timer {
                interval: 100
                onTriggered: {
                    infoText.Layout.minimumWidth=Math.max(infoText.Layout.minimumWidth,infoText.contentWidth)
                }
            }
        }
    }
}
