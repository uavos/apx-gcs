/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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

import Apx.Common

// Traffic list: red, yellow, then the rest, nearest first in each group (see TrafficPlugin.qml)
Rectangle {
    id: view

    property var traffic: apx.tools.traffic
    readonly property real fs: Style.fontSize*0.8

    // columns: icon, callsign, GNSS alt, BARO alt, distance, bearing
    readonly property real wIcon: fs*1.4
    readonly property real wCallsign: fs*5.5
    readonly property real wAlt: fs*4.5
    readonly property real wDist: fs*4.5
    readonly property real wBrg: fs*3

    function altText(v)
    {
        return isNaN(v) ? "---" : (Math.round(v) + " m")
    }

    border.width: 0
    color: "#000"

    // no internet: the message instead of the list, requests go on in background
    Row {
        id: offline
        visible: traffic.offline
        anchors.centerIn: parent
        spacing: view.height*0.08
        readonly property real h: Math.min(view.height, view.width*0.3)
        readonly property color c: "#FFC800"
        MaterialIcon {
            anchors.verticalCenter: parent.verticalCenter
            name: "wifi-off"
            size: offline.h*0.5
            color: offline.c
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            Text {
                text: qsTr("OFFLINE")
                color: offline.c
                font: apx.font_narrow(offline.h*0.36, true)
            }
            Text {
                text: qsTr("No internet connection")
                color: "#aaa"
                font: apx.font_narrow(offline.h*0.14)
            }
        }
    }

    // header: count and error
    Text {
        id: header
        visible: !traffic.offline
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Style.spacing
        text: qsTr("Traffic") + "  " + list.count
              + (traffic.status ? "  " + traffic.status : "")
        color: traffic.status ? "#FF4C00" : "#fff"
        font: apx.font_narrow(fs, true)
        elide: Text.ElideRight
    }

    // column titles
    Row {
        id: columns
        visible: !traffic.offline
        anchors.left: parent.left
        anchors.top: header.bottom
        anchors.leftMargin: Style.spacing*2
        anchors.rightMargin: Style.spacing
        spacing: fs*0.5
        Item { width: wIcon; height: 1 }
        Text { width: wCallsign; text: qsTr("Callsign"); color: "#888"; font: apx.font_narrow(fs) }
        Text { width: wAlt; text: "GNSS"; color: "#888"; font: apx.font_narrow(fs) }
        Text { width: wAlt; text: "BARO"; color: "#888"; font: apx.font_narrow(fs) }
        Text { width: wDist; text: qsTr("Dist"); color: "#888"; font: apx.font_narrow(fs) }
        Text { width: wBrg; text: qsTr("Brg"); color: "#888"; font: apx.font_narrow(fs) }
    }

    ListView {
        id: list
        visible: !traffic.offline
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: columns.bottom
        anchors.bottom: parent.bottom
        anchors.margins: Style.spacing
        clip: true
        model: traffic.aircraftSorted
        spacing: 1
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick
        currentIndex: -1
        keyNavigationEnabled: false

        ScrollBar.vertical: ScrollBar {
            width: Style.buttonSize/4
            policy: list.contentHeight > list.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        delegate: Rectangle {
            width: list.width - Style.buttonSize/4
            height: fs*1.4
            color: traffic.selected===model.hex ? "#40FFFFFF"
                 : model.level===2 ? "#40FF4C00" : "transparent"
            radius: 2

            readonly property color c: model.level===2 ? "#FF4C00"
                                     : model.level===1 ? "#FFC800"
                                     : "#fff"

            Row {
                anchors.fill: parent
                anchors.leftMargin: Style.spacing
                spacing: fs*0.5
                // category icon rotated by track, as on the map
                Item {
                    width: wIcon
                    height: parent.height
                    MaterialIcon {
                        anchors.centerIn: parent
                        name: model.icon
                        size: fs*1.2
                        color: c
                        rotation: model.track
                    }
                }
                Text { width: wCallsign; text: model.callsign; color: c; font: apx.font_narrow(fs, true); elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                Text { width: wAlt; text: altText(model.altGeomM); color: c; font: apx.font_narrow(fs); anchors.verticalCenter: parent.verticalCenter }
                Text { width: wAlt; text: model.onGround ? "GND" : altText(model.altBaroM); color: c; font: apx.font_narrow(fs); anchors.verticalCenter: parent.verticalCenter }
                Text { width: wDist; text: model.distKm.toFixed(1)+" km"; color: "#aaa"; font: apx.font_narrow(fs); anchors.verticalCenter: parent.verticalCenter }
                Text { width: wBrg; text: ("00"+Math.round(model.brg)).slice(-3)+"°"; color: "#aaa"; font: apx.font_narrow(fs); anchors.verticalCenter: parent.verticalCenter }
            }

            // click: center the map on the aircraft and pin its info panel,
            // click again: unpin
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if(traffic.selected===model.hex){
                        traffic.selected = ""
                        return
                    }
                    traffic.selected = model.hex
                    ui.map.showCoordinate(model.coordinate)
                }
            }
        }
    }
}
