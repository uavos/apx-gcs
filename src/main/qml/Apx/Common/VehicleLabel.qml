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
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3
import QtPositioning 5.6

import Apx.Common 1.0

import APX.Vehicles 1.0 as APX


Item {
    id: control

    property real fontSize: Style.fontSize
    property real fontSizeInfo: fontSize*0.6
    readonly property real dotSize: fontSize/2

    property APX.Vehicle vehicle

    property bool showDots: true

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
        Label {
            Layout.minimumWidth: control.fontSize
            horizontalAlignment: Text.AlignLeft
            font: apx.font(control.fontSize,true)
            text: callsign
            color: colorFG
        }
        Label {
            id: infoText
            Layout.fillHeight: true
            Layout.minimumWidth: control.fontSizeInfo
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            font: apx.font(control.fontSizeInfo)
            text: vehicle.info
            color: colorFG

            visible: !bLOCAL

            onImplicitWidthChanged: {
                if(vehicle.isIdentified)
                    timerWidthUpdate.start()
            }
            property Timer timerWidthUpdate: Timer {
                interval: 100
                onTriggered: {
                    infoText.Layout.minimumWidth=Math.max(infoText.Layout.minimumWidth,implicitWidth)
                }
            }
        }
    }
}
