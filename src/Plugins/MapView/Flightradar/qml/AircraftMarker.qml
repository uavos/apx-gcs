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
    anchorPoint.x: width / 2
    anchorPoint.y: height / 2

    property string callsign: "N/A"
    property real lat: 0
    property real lon: 0
    property real altitude: 0
    property real heading: 0
    property string icao: ""

    coordinate: QtPositioning.coordinate(lat, lon)

    onLatChanged: coordinate = QtPositioning.coordinate(lat, lon)
    onLonChanged: coordinate = QtPositioning.coordinate(lat, lon)

    sourceItem: Item {
        id: item
        width: 24
        height: 24

        HoverHandler { id: hover }

        Canvas {
            id: triangle
            anchors.centerIn: parent
            width: 20
            height: 28
            rotation: marker.heading

            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.clearRect(0, 0, width, height);
                ctx.save();
                
                ctx.translate(width / 2, height / 2);

                ctx.beginPath();
                ctx.moveTo(0, -4);   
                ctx.lineTo(8, 10);     
                ctx.lineTo(0, 6);      
                ctx.lineTo(-8, 10);
                ctx.closePath();

                ctx.fillStyle = "#f00";
                ctx.fill();

                ctx.lineWidth = 2;
                ctx.strokeStyle = "#fff";
                ctx.stroke();

                ctx.restore();
            }

            Behavior on rotation {
                RotationAnimation { duration: 400; easing.type: Easing.InOutQuad }
            }
        }
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: triangle.bottom
            spacing: 2

            Text {
                id: callsignText
                text: marker.callsign
                font.pixelSize: 10
                color: "#fff"
                style: Text.Outline
                styleColor: "#000"
            }

            Item {
                width: callsignText.width
                height: altitudeText.implicitHeight

                Text {
                    id: altitudeText
                    anchors.centerIn: parent
                    visible: hover.hovered
                    opacity: hover.hovered ? 1 : 0
                    text: qsTr("Alt: %1 m").arg(Math.round(marker.altitude))
                    font.pixelSize: 10
                    color: "#0f0"
                    style: Text.Outline
                    styleColor: "#000"

                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }

            Item {
                width: callsignText.width
                height: icaoText.implicitHeight

                Text {
                    id: icaoText
                    anchors.centerIn: parent
                    visible: hover.hovered
                    opacity: hover.hovered ? 1 : 0
                    text: qsTr("ICAO: %1").arg(marker.icao)
                    font.pixelSize: 10
                    color: "#0f0"
                    style: Text.Outline
                    styleColor: "#000"

                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }
    }

    Behavior on coordinate {
        enabled: true
        CoordinateAnimation { duration: 800; easing.type: Easing.InOutQuad }
    }
}
