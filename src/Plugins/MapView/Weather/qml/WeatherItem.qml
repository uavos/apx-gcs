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
import QtLocation
import QtPositioning

import Apx.Common

// one route point: arrow where the wind blows to,
// info panel on hover, pinned by a click (weather.selected)
MapQuickItem {
    id: item

    readonly property var weather: apx.tools.weather
    readonly property var map: ui.map

    // the model data is undefined while the delegate is created or destroyed (model reset),
    // a removed delegate gets index -1 (and must not match selected == -1)
    readonly property bool valid: index >= 0 && model.coordinate !== undefined && model.windDir !== undefined
    readonly property bool isWaypoint: valid && model.waypoint === true
    readonly property bool pinned: valid && weather.selected >= 0 && weather.selected === index
    readonly property bool hovered: hoverHandler.hovered
    readonly property bool showInfo: valid && (hovered || pinned)
    readonly property bool hasWind: valid && !isNaN(model.windSpeed)

    readonly property color windColor: isWaypoint ? "#FFC800" : "#00FFFF"
    readonly property real fs: Style.fontSize*0.8
    readonly property real iconSize: Style.buttonSize*(isWaypoint ? 0.6 : 0.5)

    visible: valid
    coordinate: valid ? model.coordinate : QtPositioning.coordinate()
    zoomLevel: 0
    anchorPoint.x: iconSize/2
    anchorPoint.y: iconSize/2
    z: showInfo ? 100 : 40

    function num(v, d)
    {
        return (v===undefined || isNaN(v)) ? "---" : v.toFixed(d===undefined ? 0 : d)
    }

    sourceItem: Item {
        width: iconSize
        height: iconSize

        MaterialIcon {
            id: icon
            anchors.centerIn: parent
            visible: valid
            name: hasWind ? "navigation" : "circle-small"
            size: iconSize
            color: windColor
            // the arrow points where the wind blows to
            rotation: hasWind ? model.windDir + 180 : 0
        }

        HoverHandler {
            id: hoverHandler
        }
        TapHandler {
            onTapped: if(valid) weather.selected = pinned ? -1 : index
        }

        // info panel, exists only while shown, placed to the right of the route line
        // (perpendicular to the course on the screen) so the line does not cross it
        Loader {
            id: info
            active: showInfo
            // course on the screen, clockwise from up
            readonly property real a: ((valid ? model.track : 0) - (map.bearing || 0)) * Math.PI / 180
            readonly property real px: Math.cos(a)
            readonly property real py: Math.sin(a)
            // offset from the point center along the perpendicular: the icon and half the panel extent
            readonly property real d: iconSize/2 + 4 + (width/2)*Math.abs(px) + (height/2)*Math.abs(py)
            x: parent.width/2 + px*d - width/2
            y: parent.height/2 + py*d - height/2
            sourceComponent: Rectangle {
                width: infoColumn.implicitWidth + fs
                height: infoColumn.implicitHeight + fs*0.6
                radius: 3
                color: "#D0000000"
                border.color: pinned ? "#fff" : windColor
                border.width: pinned ? 2 : 1

                TapHandler {
                    onTapped: weather.selected = -1
                }

                Column {
                    id: infoColumn
                    anchors.centerIn: parent
                    spacing: 1

                    Text {
                        text: qsTr("WP") + (model.leg+1) + (isWaypoint ? "" : " " + qsTr("leg"))
                              + "  " + num(model.dist/1000, 1) + " km"
                        color: windColor
                        font: apx.font_narrow(fs, true)
                    }
                    Text {
                        text: qsTr("ALT") + " " + num(model.altMsl) + " m MSL  (" + num(model.altAgl) + " AGL)"
                        color: "#fff"
                        font: apx.font_narrow(fs)
                    }
                    Text {
                        text: hasWind ? qsTr("WIND") + " " + ("00"+Math.round(model.windDir)).slice(-3) + "°  "
                                        + num(model.windSpeed, 1) + " m/s  " + qsTr("fcst") + " " + model.time + "Z"
                                      : qsTr("no forecast")
                        color: hasWind ? "#fff" : "#FF4C00"
                        font: apx.font_narrow(fs, true)
                    }
                }
            }
        }
    }
}
