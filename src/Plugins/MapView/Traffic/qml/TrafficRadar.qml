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
import QtCore

import Apx.Common

// TCAS-like traffic display (see TrafficRadarPlugin.qml): the aircraft by their distance and
// bearing from the unit, heading up, labelled with the altitude above the unit (m) and the
// vertical speed arrow. Symbols by the alert level: red square, yellow circle, white diamond
// (the aircraft on the ground and the own aircraft at the unit are not drawn, the ground
// traffic of an airport would be a cluster at the unit). The threats carry the time to
// the zone, s.
// The blue arrow by an aircraft is its track. The aircraft under the compass band (the
// outer ring with the numbers) are not drawn. The range (the outer ring) is edited in
// the field at the top right, apart from the request radius of the plugin. View: all around
// (the unit in the center) or the sector ahead (the unit at the bottom), the buttons at the
// bottom (as in the Signals widget). The range and the view are kept in the settings.
// A click on an aircraft highlights it (white frames) and centers the map on it.
// Nothing is shown without the unit position
Rectangle {
    id: radar

    property var traffic: apx.tools.traffic
    readonly property var mandala: apx.fleet.current.mandala
    readonly property real fs: Style.fontSize*0.7

    implicitWidth: Style.buttonSize*13
    implicitHeight: Style.buttonSize*11
    border.width: 0
    color: "#000"
    clip: true

    // heading up with the unit, north up with the map center as the origin
    readonly property bool unitOrigin: traffic.unitOrigin
    readonly property real heading: unitOrigin ? mandala.est.att.yaw.value : 0
    readonly property var unit: apx.fleet.current

    // view: all around or the sector ahead
    property bool sector: false
    // range (the outer ring), km
    property int rangeKm: 20
    readonly property int rangeMin: 1
    readonly property int rangeMax: 400
    Settings {
        category: "traffic-radar"
        property alias sector: radar.sector
        property alias rangeKm: radar.rangeKm
    }

    // the unit symbol and the pixel scale; in the sector view the compass arc (+-compassDeg)
    // fits the width, the rings beyond it are clipped by the edges
    readonly property real compassDeg: 55
    readonly property real cx: width/2
    readonly property real cy: sector ? height - fs*2.4 : height/2
    readonly property real rpx: sector ? Math.min(cy - fs*2.8, (width/2 - fs*0.8)/Math.sin(compassDeg*Math.PI/180))
                                       : Math.min(width, height)/2 - fs*2.4
    readonly property real pxPerKm: rpx/rangeKm

    onHeadingChanged: rings.requestPaint()
    onRangeKmChanged: rings.requestPaint()
    onSectorChanged: rings.requestPaint()
    onWidthChanged: rings.requestPaint()
    onHeightChanged: rings.requestPaint()

    // range rings, the alert zones and the compass arc
    Canvas {
        id: rings
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.lineWidth = 1
            // the sector ahead: the rings are the half circles above the unit,
            // the compass arc spans +-compassDeg
            function ring(r, style, dash, halfDeg){
                var h = halfDeg*Math.PI/180
                ctx.strokeStyle = style
                ctx.setLineDash(dash)
                ctx.beginPath()
                if(sector) ctx.arc(cx, cy, r, -Math.PI/2 - h, -Math.PI/2 + h)
                else ctx.arc(cx, cy, r, 0, Math.PI*2)
                ctx.stroke()
            }
            // dashed rings at quarters of the range
            for(var q=1; q<4; ++q)
                ring(rpx*q/4, "#666", [4, 4], 90)
            // the alert zones
            ring(traffic.proxRadiusM/1000*pxPerKm, "#60FFC800", [4, 4], 90)
            ring(traffic.alertRadiusM/1000*pxPerKm, "#80FF4C00", [4, 4], 90)
            // the compass: the outer arc with ticks every 10 deg, numbers every 30 (tens)
            ring(rpx, "#ccc", [], compassDeg)
            ctx.fillStyle = "#ccc"
            ctx.font = Math.round(fs) + "px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            for(var d=0; d<360; d+=10){
                var a = (d - heading)*Math.PI/180
                var s = Math.sin(a), c = -Math.cos(a)
                if(sector && Math.cos(a) < Math.cos(compassDeg*Math.PI/180) - 0.001) continue // off the arc
                var len = (d%30===0) ? fs*0.6 : fs*0.3
                ctx.beginPath()
                ctx.moveTo(cx + rpx*s, cy + rpx*c)
                ctx.lineTo(cx + (rpx-len)*s, cy + (rpx-len)*c)
                ctx.stroke()
                if(d%30===0)
                    ctx.fillText(d===0 ? "N" : (d/10).toString(), cx + (rpx-fs*1.2)*s, cy + (rpx-fs*1.2)*c)
            }
        }
    }

    // the unit (or the map center), a click centers the map on the unit
    MaterialIcon {
        x: cx - width/2
        y: cy - height/2
        name: unitOrigin ? "airplane" : "crosshairs-gps"
        size: fs*1.8
        color: "#fff"
        MouseArea {
            anchors.fill: parent
            anchors.margins: -Style.spacing
            enabled: unitOrigin
            onClicked: {
                var c = unit.coordinate
                if(ui.map.follow) ui.map.centerOn(c) // showCoordinate is off while following
                else ui.map.showCoordinate(c)
            }
        }
    }

    // heading readout
    Rectangle {
        x: cx - width/2
        y: cy - rpx - height - 2
        width: hdgText.implicitWidth + fs
        height: hdgText.implicitHeight + 2
        color: "#000"
        border.color: "#ccc"
        border.width: 1
        Text {
            id: hdgText
            anchors.centerIn: parent
            text: unitOrigin ? ("00"+Math.round((heading%360+360)%360)).slice(-3) : "N"
            color: "#fff"
            font: apx.font_narrow(fs, true)
        }
    }

    // the range, edited in place
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Style.spacing
        width: radiusRow.implicitWidth + fs
        height: fs*1.5
        color: "#222"
        border.color: radiusInput.activeFocus ? "#FFC800" : "#555"
        border.width: 1
        radius: 2
        Row {
            id: radiusRow
            anchors.centerIn: parent
            spacing: fs*0.3
            TextInput {
                id: radiusInput
                width: fs*2.5
                text: radar.rangeKm
                color: "#fff"
                font: apx.font_narrow(fs, true)
                horizontalAlignment: TextInput.AlignRight
                selectByMouse: true
                validator: IntValidator { bottom: radar.rangeMin; top: radar.rangeMax }
                // applied on Enter or when the focus leaves, reverted otherwise
                onEditingFinished: {
                    if(acceptableInput) radar.rangeKm = parseInt(text)
                    text = Qt.binding(function(){ return radar.rangeKm })
                    focus = false
                }
                Keys.onEscapePressed: {
                    text = Qt.binding(function(){ return radar.rangeKm })
                    focus = false
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "km"
                color: "#ccc"
                font: apx.font_narrow(fs)
            }
        }
    }
    // the view buttons (the bottom row, as the pages of the Signals widget)
    readonly property real btnSize: Style.buttonSize*0.7
    Row {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Style.spacing
        spacing: Style.spacing
        TextButton {
            size: btnSize
            text: "360"
            toolTip: qsTr("All around")
            checkable: true
            checked: !sector
            textColor: checked ? "#FFC800" : "#ccc"
            onTriggered: sector = false
        }
        TextButton {
            size: btnSize
            text: qsTr("SECT")
            toolTip: qsTr("Sector ahead")
            checkable: true
            checked: sector
            textColor: checked ? "#FFC800" : "#ccc"
            onTriggered: sector = true
        }
    }
    // the error
    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Style.spacing
        visible: traffic.offline || traffic.status
        text: traffic.offline ? qsTr("OFFLINE") : traffic.status
        color: "#FF4C00"
        font: apx.font_narrow(fs)
    }

    // no unit position: nothing to relate the traffic to
    Text {
        visible: !unitOrigin
        anchors.centerIn: parent
        text: qsTr("NO UNIT")
        color: "#FFC800"
        font: apx.font_narrow(fs*2, true)
    }

    // the aircraft
    Repeater {
        id: aircraft
        model: unitOrigin ? traffic.aircraft : null
        delegate: Item {
            id: target
            readonly property real ang: (model.brg - radar.heading)*Math.PI/180
            readonly property real r: model.distKm*radar.pxPerKm
            readonly property bool highlighted: traffic.highlighted === model.hex
            // inside the compass band (the scale is kept, the band hides them), ahead in the
            // sector view, not the own aircraft, airborne only
            visible: !model.own && !model.onGround && r <= radar.rpx - radar.fs*1.8 && (!radar.sector || Math.cos(ang) >= -0.01)
            x: radar.cx + r*Math.sin(ang)
            y: radar.cy - r*Math.cos(ang)
            z: highlighted ? 10 : model.level

            readonly property color c: model.level===2 ? "#FF4C00"
                                     : model.level===1 ? "#FFC800"
                                     : "#fff"
            readonly property real s: fs*0.9

            // the track (heading up): a thin arrow out of the symbol, as the vector
            // leader of a radar display (when the track is reported)
            Item {
                id: arrow
                visible: model.trackValid
                rotation: model.track - radar.heading
                transformOrigin: Item.TopLeft
                readonly property real gap: s*0.8  // from the symbol center to the tail
                readonly property real len: s*1.5
                readonly property color ac: "#2E9BFF"
                Rectangle { // the shaft
                    x: -0.75
                    y: -(parent.gap + parent.len)
                    width: 1.5
                    height: parent.len
                    color: parent.ac
                }
                Repeater { // the head: two wings from the tip
                    model: [-35, 35]
                    delegate: Rectangle {
                        x: -0.75
                        y: -(arrow.gap + arrow.len)
                        width: 1.5
                        height: s*0.55
                        color: arrow.ac
                        transformOrigin: Item.Top
                        rotation: modelData
                    }
                }
            }

            // red: filled square, yellow: filled circle, other: hollow diamond
            Rectangle {
                x: -s/2
                y: -s/2
                width: s
                height: s
                radius: model.level===1 ? s/2 : 0
                rotation: model.level===0 ? 45 : 0
                color: model.level>0 ? c : "transparent"
                border.color: c
                border.width: model.level>0 ? 0 : 1.5
            }
            // highlighted: a thin white frame right around the symbol (turned with the diamond)
            Rectangle {
                visible: highlighted
                anchors.centerIn: parent
                width: s*1.25
                height: s*1.25
                radius: model.level===1 ? width/2 : 0
                rotation: model.level===0 ? 45 : 0
                color: "transparent"
                border.color: "#fff"
                border.width: 1
            }
            // altitude above the unit, m (above the symbol when above) and the climb or
            // descent arrow (an icon: the narrow font has no arrow glyphs)
            Row {
                visible: !isNaN(model.dAltM)
                x: s*0.9
                y: model.dAltM >= 0 ? -height-s*0.1 : s*0.1
                spacing: 0
                Text {
                    text: (model.dAltM >= 0 ? "+" : "-") + Math.round(Math.abs(model.dAltM))
                    color: c
                    font: apx.font_narrow(fs)
                }
                MaterialIcon {
                    visible: Math.abs(model.vsMs) > 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    name: model.vsMs > 0 ? "arrow-up-bold" : "arrow-down-bold"
                    size: fs*0.9
                    color: c
                }
            }
            // the time to the zone, s (the threats on their way, inside the zone the color says it)
            Text {
                visible: model.level > 0 && isFinite(model.tauS) && model.tauS >= 0.5
                x: -s*0.9 - width
                y: -height/2
                text: Math.round(model.tauS) + "s"
                color: c
                font: apx.font_narrow(fs)
            }
            // click: highlight the aircraft (here, on the map and in the list, click again
            // to drop) and center the map on it; the info panel is on hover on the map
            MouseArea {
                x: -s
                y: -s
                width: s*2
                height: s*2
                onClicked: {
                    traffic.selected = "" // a panel pinned on the map is unpinned
                    traffic.highlighted = traffic.highlighted===model.hex ? "" : model.hex
                    ui.map.showCoordinate(model.coordinate)
                }
            }
        }
    }
}
