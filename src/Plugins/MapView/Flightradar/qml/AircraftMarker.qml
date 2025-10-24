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
import QtLocation
import QtPositioning

MapQuickItem {
    id: marker
    z: 100
    anchorPoint.x: circle.width / 2
    anchorPoint.y: circle.height / 2

    property string callsign: ""
    property real lat: 0
    property real lon: 0
    property real altitude: 0
    property real heading: 0

    coordinate: QtPositioning.coordinate(lat, lon)
    
    onLatChanged: coordinate = QtPositioning.coordinate(lat, lon)
    onLonChanged: coordinate = QtPositioning.coordinate(lat, lon)

    sourceItem: Item {
        width: 24
        height: 24

        Rectangle {
            id: circle
            anchors.centerIn: parent
            width: 8
            height: 8
            radius: width / 2
            color: "#f00"
            border.color: "#fff"
            border.width: 2
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: circle.bottom
            spacing: 2

            Text {
                text: marker.callsign
                font.pixelSize: 10
                color: "white"
                style: Text.Outline
                styleColor: "black"
            }            
        }
    }

    Behavior on coordinate {
        enabled: true
        CoordinateAnimation { duration: 800; easing.type: Easing.InOutQuad }
    }
}
