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

// ADS-B aircraft: icon by category rotated by track,
// info panel on hover, pinned by a click (traffic.selected)
MapQuickItem {
    id: item

    readonly property var traffic: apx.tools.traffic
    readonly property bool pinned: traffic.selected === model.hex
    readonly property bool hovered: hoverHandler.hovered
    readonly property bool showInfo: hovered || pinned

    readonly property color levelColor: model.level===2 ? "#FF4C00"
                                      : model.level===1 ? "#FFC800"
                                      : "#00FFFF"
    readonly property real fs: Style.fontSize*0.8

    coordinate: model.coordinate
    zoomLevel: 0
    anchorPoint.x: icon.width/2
    anchorPoint.y: icon.height/2
    z: showInfo ? 100 : 50

    function altText(v)
    {
        return isNaN(v) ? "---" : (Math.round(v) + " m")
    }

    sourceItem: Item {
        width: icon.width
        height: icon.height

        MaterialIcon {
            id: icon
            name: model.icon
            size: Style.buttonSize*0.8*model.iconScale
            color: levelColor
            rotation: model.track
            style: Text.Outline
            styleColor: showInfo ? "#fff" : "#000"
        }

        HoverHandler {
            id: hoverHandler
        }
        TapHandler {
            onTapped: traffic.selected = pinned ? "" : model.hex
        }

        // info panel
        Rectangle {
            visible: showInfo
            anchors.left: icon.right
            anchors.leftMargin: 2
            anchors.verticalCenter: icon.verticalCenter
            width: infoColumn.implicitWidth + fs
            height: infoColumn.implicitHeight + fs*0.6
            radius: 3
            color: "#D0000000"
            border.color: pinned ? "#fff" : levelColor
            border.width: pinned ? 2 : 1

            Column {
                id: infoColumn
                anchors.centerIn: parent
                spacing: 1

                Text {
                    text: model.callsign
                    color: levelColor
                    font: apx.font_narrow(fs, true)
                }
                Text {
                    text: model.categoryText + (model.type ? "  " + model.type : "")
                    color: "#ccc"
                    font: apx.font_narrow(fs)
                }
                Text {
                    text: model.onGround ? "GND"
                                         : ("ALT " + model.altSrc + " " + altText(model.altM)
                                            + (model.vsMs > 0.5 ? "  \u2191" + model.vsMs.toFixed(1) + " m/s"
                                              : model.vsMs < -0.5 ? "  \u2193" + (-model.vsMs).toFixed(1) + " m/s"
                                              : ""))
                    color: "#fff"
                    font: apx.font_narrow(fs)
                }
                Text {
                    visible: !model.onGround && !isNaN(model.altBaroM) && !isNaN(model.altGeomM)
                    text: "BARO " + altText(model.altBaroM) + "  GNSS " + altText(model.altGeomM)
                    color: "#aaa"
                    font: apx.font_narrow(fs)
                }
                Text {
                    text: (model.gsKt > 0 ? Math.round(model.gsKt) + " kt  " : "")
                          + model.distKm.toFixed(1) + " km  "
                          + ("00"+Math.round(model.brg)).slice(-3) + "\u00B0"
                    color: "#aaa"
                    font: apx.font_narrow(fs)
                }
            }
        }
    }
}
