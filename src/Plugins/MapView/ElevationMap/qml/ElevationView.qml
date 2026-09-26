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
import QtCharts
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Shapes
import QtQml

import QtQml.Models

import Apx.Common

import APX.Fleet as APX
import APX.Mission
import Apx.Elevation 1.0

// Terrain elevation chart, embedded into the main layout (see ElevationPlugin.qml)
Rectangle {
    id: elevationView

    // half of the main window width, height like the Signals panel
    // (chart 110*ui.scale + set row + buttons row)
    implicitWidth: Window.width > 0 ? Window.width * 0.5 : Style.buttonSize*16
    implicitHeight: 134 * ui.scale + Style.fontSize + Style.spacing * 3
    border.width: 0
    color: "#000"

    property APX.Unit unit: apx.fleet.current
    readonly property Mission mission: unit.mission
    readonly property bool empty: mission.empty
    property var elevationmap: apx.tools.elevationmap
    property var use: elevationmap ? elevationmap.use.value : false 
    property var elevationPlugin: apx.settings.application.plugins.elevationmap
    property var pluginOn: elevationPlugin ? elevationPlugin.value : false
    property var chartOn: elevationmap ? elevationmap.active : false
    property var name: qsTr("Terrain elevation")
    property var disabled: qsTr("(disabled)")
    property real zoomFactor: 1.5   // per button click
    property real minScale: 1
    property real maxScale: 100
    property real chartScale: minScale

    // Zoom and pan are done through the X axis range, the chart item itself
    // always stays the size of the window (one texture, no per-zoom reallocation)
    readonly property real fullSpan: Math.max(missionLength, 1000)

    // Segment start distances computed synchronously from waypoint path lengths.
    // MissionGroup updates totalDistance with a 1 s timer, so while a waypoint is
    // being dragged its own distance is already new but totalDistance is stale;
    // deriving offsets here keeps profile segments and markers consistent.
    property var segmentStarts: []
    property real missionLength: 0
    property real pendingLength: 0
    // flight altitude line: [distance m, height AMSL m] per point, start point first
    property var missionLine: []
    function updateSegmentStarts() {
        var p1 = mission.coordinate
        var p2 = mission.startPoint
        var startHmsl = Math.round(mission.startElevation)
        var acc = (p1.isValid && p2.isValid) ? p1.distanceTo(p2) : 0
        var starts = []
        var line = []
        if(p2.isValid && !isNaN(mission.startElevation))
            line.push([0, mission.startElevation])
        var group = mission.wp
        for(var i = 0; i < group.size; ++i) {
            starts.push(acc)
            var wp = group.child(i)
            if(!wp)
                continue
            acc += wp.distance
            var altFact = wp.child("altitude")
            var amslFact = wp.child("amsl")
            var alt = altFact ? Number(altFact.value) : 0
            line.push([acc, (amslFact && amslFact.value) ? alt : alt + startHmsl])
        }
        segmentStarts = starts
        missionLine = line
        pendingLength = acc
        markers.refresh()
        if(missionLength == 0 || !lengthTimer.running)
            lengthTimer.restart()
    }
    // the axis range (a QtCharts relayout) is updated at most a few times per second
    Timer {
        id: lengthTimer
        interval: 250
        onTriggered: missionLength = pendingLength
    }
    function scheduleSegmentStarts() { Qt.callLater(updateSegmentStarts) }
    function segmentStart(index, fallback) {
        var v = segmentStarts[index]
        return v === undefined ? fallback : v
    }
    Connections {
        target: mission.wp
        function onSizeChanged() { scheduleSegmentStarts() }
    }
    Connections {
        target: mission
        function onStartPointChanged() { scheduleSegmentStarts() }
        function onCoordinateChanged() { scheduleSegmentStarts() }
    }
    Component.onCompleted: updateSegmentStarts()
    property real viewSpan: fullSpan
    property real viewStart: 0
    readonly property real pixelsPerMeter: Math.max(chartView.plotArea.width, 1) / viewSpan

    function xOf(distance) {
        return chartView.plotArea.x + (distance - viewStart) * pixelsPerMeter
    }
    function clampStart(s) {
        return Math.min(Math.max(s, 0), Math.max(fullSpan - viewSpan, 0))
    }
    // zoom by factor keeping the distance under anchorPx (window x) in place;
    // without anchor the center of the plot is kept
    function zoomAt(factor, anchorPx) {
        var s = Math.min(Math.max(chartScale * factor, minScale), maxScale)
        if(s === chartScale)
            return
        var plotX = chartView.plotArea.x
        if(anchorPx === undefined || anchorPx < plotX || anchorPx > plotX + chartView.plotArea.width)
            anchorPx = plotX + chartView.plotArea.width / 2
        var anchorDistance = viewStart + (anchorPx - plotX) / pixelsPerMeter
        var span = fullSpan / s
        var ppm = Math.max(chartView.plotArea.width, 1) / span
        chartScale = s
        viewSpan = span
        viewStart = clampStart(anchorDistance - (anchorPx - plotX) / ppm)
    }
    onFullSpanChanged: {
        viewSpan = fullSpan / chartScale
        viewStart = clampStart(viewStart)
    }

    onEmptyChanged: if(empty) resetChartScale()

    // fonts and colors follow the Signals plugin charts
    readonly property font axisFont: apx.font_narrow(Style.fontSize * 0.65)
    readonly property font labelFont: apx.font_narrow(Style.fontSize * 0.8)

    Label {
        anchors.centerIn: parent
        text: name + " " + disabled
        font: labelFont
        visible: !elevationView.chartOn
    }

    Label {
        id: axisYLabel
        anchors.top: parent.bottom
        width: parent.height
        text: qsTr("Height AMSL, %1").arg("m")
        font: labelFont
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        transformOrigin: Item.TopLeft
        rotation: -90
        visible: elevationView.chartOn
    }

    Label {
        id: axisXLabel
        anchors.bottom: parent.bottom
        width: parent.width
        text: qsTr("Distance, %1").arg("m")
        font: labelFont
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        visible: elevationView.chartOn
    }

    Rectangle {
        id: alarm
        property int margin: 5

        height: txt.height
        width: icon.width + txt.width + 2*margin
        color: "#ff0000"
        radius: 2
        border.width: radius
        border.color: "#ffffff"
        visible: mission.collision && elevationView.chartOn
        anchors {
            top: parent.top
            left: parent.left
            topMargin: margin
            leftMargin: Style.buttonSize + margin // the plugin frame puts its maximize button here
        }
        MaterialIcon {
            id: icon
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: alarm.margin/2
            name: "alert-circle"
            color: txt.color
            size: txt.font.pixelSize
        }
        Text {
            id: txt
            text: qsTr("Alarm")
            color: "#ffffff"
            font: elevationView.labelFont
            anchors.left: icon.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: alarm.margin/2
        }
    }

    Item {
        id: chartItem
        height: elevationView.height
        width: elevationView.width
        visible: elevationView.chartOn

        ChartView {
            id: chartView
            property int margin: 5
            property var startPoint: mission.startPoint
            property var dist: mission.wp.distance
            property var minHeight: mission.minHeight
            property var maxHeight: mission.maxHeight
            property var distance:  dist
            anchors.fill: parent
            margins.top: alarm.height
            margins.right: margin
            margins.bottom: margin
            margins.left: 2*margin
            backgroundColor: "transparent"
            legend.visible: false
            antialiasing: true

            onDistChanged: updateDistance()

            ValueAxis {
                id: axisX
                min: elevationView.viewStart
                max: elevationView.viewStart + elevationView.viewSpan
                lineVisible: true
                labelsFont: elevationView.axisFont
                labelsColor: "white"
                gridVisible: false
                tickCount: 11 // the visible range shrinks with zoom, the chart width does not
                labelFormat: "%.0f"
            }
            ValueAxis {
                id: axisY
                min: chartView.minHeight
                max: Math.ceil(mission.maxHeight/10)*10
                lineVisible: true
                labelsFont: elevationView.axisFont
                labelsColor: "white"
                gridLineColor: "#555"
                tickCount: 5
                labelFormat: "%.0f"
            }
            // empty series keeps the axes attached; the flight altitude line itself is
            // drawn by missionLineShape (scene graph), QtCharts only repaints on axis changes
            LineSeries {
                id: lineSeries
                axisX: axisX
                axisY: axisY
            }
            function updateDistance()
            {
                var p1 = mission.coordinate
                var p2 = mission.startPoint
                var rwLength = p1.isValid && p2.isValid ? p1.distanceTo(p2) : 0
                distance = dist + rwLength;
            }
        }

        // Start point
        Item {
            id: startPoint
            property var coordinate: mission.startPoint
            property var elevationmap: apx.tools.elevationmap
            property var chartHeight: chartView.plotArea.height
            property var scaleY: axisY.max/chartHeight
            property var startElevation: mission.startElevation
            property var hStartPoint: !isNaN(startElevation)?(startElevation/scaleY):0

            visible: mission.startPoint.isValid && x >= chartView.plotArea.x
            x: elevationView.xOf(0)
            y: chartView.plotArea.y + chartHeight
            z: 1

            Rectangle {
                id: takeOffPoint
                height: 16
                width: height
                x: -width/2
                y: -height/2 - startPoint.hStartPoint
                radius: height/8
                color: "#3779c5"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("R")
                    font.pixelSize: 12
                    font.bold: true
                    color: "#ffffff"
                }
            }

            onStartElevationChanged: elevationView.scheduleSegmentStarts()
        }

        // Flight altitude line (start point + waypoints), GPU rendered
        Shape {
            id: missionLineShape
            x: chartView.plotArea.x
            y: chartView.plotArea.y
            width: chartView.plotArea.width
            height: chartView.plotArea.height
            clip: true
            z: 0.5
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: "#209fdf"
                strokeWidth: 2
                fillColor: "transparent"
                joinStyle: ShapePath.RoundJoin
                PathPolyline {
                    path: {
                        var pts = []
                        var line = elevationView.missionLine
                        var ppm = elevationView.pixelsPerMeter
                        var yRange = Math.max(axisY.max - axisY.min, 1)
                        var h = chartView.plotArea.height
                        for(var i = 0; i < line.length; ++i) {
                            pts.push(Qt.point((line[i][0] - elevationView.viewStart) * ppm,
                                              h - (line[i][1] - axisY.min) / yRange * h))
                        }
                        return pts
                    }
                }
            }
        }
        // Waypoint markers (numbers, vertical lines, altitude drag), GPU rendered
        WaypointMarkersItem {
            id: markers
            // same area as hoverArea: the plot plus space above it for the top markers
            x: chartView.plotArea.x
            y: chartView.plotArea.y - hoverArea.topExtent
            width: chartView.plotArea.width
            height: chartView.plotArea.height + hoverArea.topExtent
            topPadding: hoverArea.topExtent
            z: 1
            group: mission.wp
            viewStart: elevationView.viewStart
            viewSpan: elevationView.viewSpan
            minHeight: axisY.min
            maxHeight: axisY.max
            onChanged: elevationView.scheduleSegmentStarts()
        }
        Loader {
            id: epLoader
            active: elevationView.visible && elevationView.chartOn
            anchors.fill: parent
            asynchronous: true
            sourceComponent: Component { ElevationChart { } }
        }

        // terrain elevation under the mouse (HoverHandler never blocks other items)
        Item {
            id: hoverArea
            // extends above the plot: a marker at the top altitude sticks out of it
            readonly property real topExtent: 24
            x: chartView.plotArea.x
            y: chartView.plotArea.y - topExtent
            width: chartView.plotArea.width
            height: chartView.plotArea.height + topExtent
            HoverHandler {
                id: hoverHandler
                cursorShape: markers.hovered ? Qt.PointingHandCursor : Qt.ArrowCursor
                onPointChanged: {
                    if(!hovered)
                        return
                    markers.hoverAt(point.position.x, point.position.y) // same origin as hoverArea
                    if(markers.hovered || point.position.y < hoverArea.topExtent)
                        hoverCursor.hide()
                    else
                        hoverCursor.update(point.position.x)
                }
                onHoveredChanged: {
                    if(hovered)
                        return
                    hoverCursor.hide()
                    markers.hoverLeave()
                }
            }
        }
        Item {
            id: hoverCursor
            property real distance: 0
            property real elevation: NaN
            visible: false
            z: 2

            function update(px) {
                var d = elevationView.viewStart + px / elevationView.pixelsPerMeter
                var e = elevationView.terrainElevationAt(d)
                if(isNaN(e)) {
                    hide()
                    return
                }
                distance = d
                elevation = e
                x = hoverArea.x + px
                visible = true
            }
            function hide() { visible = false }

            // terrain point in window coordinates
            property real terrainY: chartView.plotArea.y + chartView.plotArea.height
                                    - (elevation - axisY.min) / Math.max(axisY.max - axisY.min, 1)
                                      * chartView.plotArea.height
            Rectangle {
                width: 1
                y: chartView.plotArea.y
                height: Math.max(hoverCursor.terrainY - y, 0)
                color: "#a0ffffff"
            }
            Rectangle {
                id: hoverDot
                width: 9
                height: width
                radius: width / 2
                x: -width / 2 + 0.5
                y: hoverCursor.terrainY - height / 2
                color: "#ffffff"
                border.color: "#00c000"
                border.width: 2
            }
            Rectangle {
                id: hoverLabel
                property real anchorX: hoverCursor.x
                // keep the label inside the window
                x: Math.min(4, elevationView.width - anchorX - width - 4)
                y: chartView.plotArea.y + 4
                width: hoverText.implicitWidth + 12
                height: hoverText.implicitHeight + 8
                radius: 3
                color: "#d0202020"
                border.color: "#80ffffff"
                border.width: 1
                Text {
                    id: hoverText
                    anchors.centerIn: parent
                    color: "#ffffff"
                    font: elevationView.labelFont
                    text: qsTr("Terrain %1").arg(apx.distanceToString(Math.max(0, Math.round(hoverCursor.elevation))))
                          + "\n" + qsTr("Distance %1").arg(apx.distanceToString(Math.max(0, Math.round(hoverCursor.distance))))
                }
            }
        }
    }

    // terrain elevation at the mission distance, NaN when no profile covers it
    function terrainElevationAt(distance) {
        var repeater = epLoader.item
        if(!repeater)
            return NaN
        for(var i = 0; i < repeater.count; ++i) {
            var item = repeater.itemAt(i)
            if(!item || !item.visible)
                continue
            var e = item.elevationAt(distance)
            if(!isNaN(e))
                return e
        }
        return NaN
    }
    
    // legend of the chart lines (top right, zoom is done with the wheel, double click resets it)
    Row {
        id: legend
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Style.spacing*2
        spacing: Style.spacing*3
        visible: elevationView.chartOn
        z: 5
        readonly property font fnt: elevationView.axisFont
        Row {
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter
            Rectangle { width: 16; height: 8; anchors.verticalCenter: parent.verticalCenter
                        color: "#4000ff00"; border.color: "#00ff00"; border.width: 1 }
            Text { text: qsTr("Terrain, corridor max"); color: "white"; font: legend.fnt }
        }
        Row {
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter
            Rectangle { width: 16; height: 2; anchors.verticalCenter: parent.verticalCenter; color: "#ffb000" }
            Text { text: qsTr("Terrain, corridor min"); color: "white"; font: legend.fnt }
        }
        Row {
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter
            Rectangle { width: 16; height: 2; anchors.verticalCenter: parent.verticalCenter; color: "#209fdf" }
            Text { text: qsTr("Flight altitude"); color: "white"; font: legend.fnt }
        }
        Row {
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter
            Rectangle { width: 16; height: 8; anchors.verticalCenter: parent.verticalCenter
                        color: "#40ff0000"; border.color: "#ff0000"; border.width: 1 }
            Text { text: qsTr("Collision"); color: "white"; font: legend.fnt }
        }
        // plugin menu (use, path, corridor, unit AGL)
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            size: legend.fnt.pixelSize * 1.6
            iconName: "menu"
            toolTip: qsTr("Elevation map settings")
            onTriggered: if(elevationmap) elevationmap.trigger()
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onDoubleTapped: resetChartScale()
    }

    WheelHandler {
        id: wheelHandler
        target: null
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function (event) {
            // one mouse notch (120) = x1.25, trackpad gives small smooth steps;
            // zoom around the mouse position
            zoomAt(Math.pow(1.25, event.angleDelta.y / 120), event.x)
        }
    }

    DragHandler {
        id: dragHandler
        target: null
        acceptedButtons: Qt.LeftButton
        property real startView: 0
        onActiveChanged: if(active) startView = elevationView.viewStart
        onActiveTranslationChanged: {
            if(!active)
                return
            elevationView.viewStart = elevationView.clampStart(startView - activeTranslation.x / elevationView.pixelsPerMeter)
        }
    }

    function resetChartScale() {
        chartScale = 1
        viewSpan = fullSpan
        viewStart = 0
    }
}

