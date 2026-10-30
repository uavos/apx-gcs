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

// Traffic list, nearest first, one line per aircraft (see TrafficPlugin.qml)
Rectangle {
    id: view

    property var traffic: apx.tools.traffic
    readonly property real fs: Style.fontSize*0.8

    border.width: 0
    color: "#000"


    // header: count and error
    Text {
        id: header
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

    ListView {
        id: list
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
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
            color: model.level===2 ? "#40FF4C00" : "transparent"
            radius: 2

            readonly property color c: model.level===2 ? "#FF4C00"
                                     : model.level===1 ? "#FFC800"
                                     : "#fff"

            Row {
                anchors.fill: parent
                anchors.leftMargin: Style.spacing
                spacing: fs*0.5
                Text { width: fs*2.8; text: model.number; color: c; font: apx.font_narrow(fs, true); horizontalAlignment: Text.AlignHCenter }
                Text { width: fs*6; text: model.callsign; color: c; font: apx.font_narrow(fs, true); elide: Text.ElideRight }
                Text { width: fs*5; text: model.altFt>0 ? Math.round(model.altFt)+" ft" : "GND"; color: c; font: apx.font_narrow(fs) }
                Text { width: fs*5; text: model.distKm.toFixed(1)+" km"; color: "#aaa"; font: apx.font_narrow(fs) }
                Text { width: fs*3; text: ("00"+Math.round(model.brg)).slice(-3)+"°"; color: "#aaa"; font: apx.font_narrow(fs) }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: ui.map.showCoordinate(model.coordinate)
            }
        }
    }
}
