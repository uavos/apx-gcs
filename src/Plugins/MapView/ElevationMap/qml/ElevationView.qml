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
import QtQuick.Window
import QtQuick.Layouts
import QtQml

import QtQml.Models

import Apx.Common

import APX.Fleet as APX
import APX.Mission

Window {
    id: elevationView
    property APX.Unit unit: apx.fleet.current
    readonly property Mission mission: unit.mission
    readonly property bool empty: mission.empty
    property var elevationmap: apx.tools.elevationmap
    property var use: elevationmap ? elevationmap.use.value : false 
    property var elevationPlugin: apx.settings.application.plugins.elevationmap
    property var pluginOn: elevationPlugin ? elevationPlugin.value : false
    property var chartOn: use && pluginOn
    property var name: qsTr("Terrain elevation")
    property var disabled: qsTr("(disabled)")
    property real zoomFactor: 1.5   // per button click
    property real minScale: 1
    property real maxScale: 100
    property real chartScale: minScale

    // Zoom and pan are done through the X axis range, the chart item itself
    // always stays the size of the window (one texture, no per-zoom reallocation)
    readonly property real fullSpan: Math.max(chartView.distance, 1000)
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

    flags: Qt.WindowStaysOnTopHint
    width: Screen.desktopAvailableWidth - 50
    height: 200
    maximumHeight: Screen.desktopAvailableHeight / 3
    maximumWidth: Screen.desktopAvailableWidth
    minimumHeight: 200
    minimumWidth: 600
    title: chartOn ? name : name + " " + disabled
    color: "#cc000000"
    visible: true
    x: 25
    y: Screen.desktopAvailableHeight - height - 50

    onClosing: plugin.active=false
    onVisibleChanged: timer.restart()
    onEmptyChanged: if(empty) resetChartScale()
    onVisibilityChanged: {
        if (visibility === Window.Maximized)
            height = maximumHeight
    }

    Timer {
        id: timer
        interval: 500
        onTriggered: epLoader.active=elevationView.visible
    }

    Label {
        id: axisYLabel
        anchors.top: parent.bottom
        width: parent.height
        text: qsTr("Height AMSL, %1").arg("m")
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
        visible: mission.collision && use
        anchors {
            top: parent.top
            left: parent.left
            topMargin: margin
            leftMargin: margin
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
            font.bold: true
            font.pixelSize: Style.fontSize*0.8
            anchors.left: icon.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: alarm.margin/2
        }
        SequentialAnimation {
            // an infinite animation on a hidden item still forces the window
            // to be re-rendered every frame - run it only while the alarm is shown
            running: alarm.visible
            loops: Animation.Infinite
            PropertyAnimation {
                target: alarm
                property: "opacity"
                    to: 0.5
                duration: 1500
            }
            PropertyAnimation {
                target: alarm
                property: "opacity"
                to: 1
                duration: 1500
            }
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
                labelsFont.family: axisXLabel.font.family
                labelsFont.pointSize: axisXLabel.font.pointSize
                labelsColor: axisXLabel.color
                gridVisible: false
                tickCount: 11 // the visible range shrinks with zoom, the chart width does not
                labelFormat: "%.0f"
            }
            ValueAxis {
                id: axisY
                min: chartView.minHeight
                max: Math.ceil(mission.maxHeight/10)*10
                lineVisible: true
                labelsFont.family: axisYLabel.font.family
                labelsFont.pointSize: axisYLabel.font.pointSize
                labelsColor: axisYLabel.color
                gridLineColor: "#40ffffff"
                tickCount: 5
                labelFormat: "%.0f"
            }
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

            Component.onCompleted: initStartPoint()
            onVisibleChanged: initStartPoint()
            onCoordinateChanged: updateStartPoint()
            onStartElevationChanged: updateStartPoint()

            function initStartPoint()
            {
                if(isNaN(startElevation))
                    return
                if(visible)
                    lineSeries.insert(-1, 0, startElevation)
                else
                    if(lineSeries.count > 0) 
                        lineSeries.remove(0)
            }
            function updateStartPoint()
            {
                if(!visible)
                    return
                if(isNaN(startElevation))
                    return
                var point = lineSeries.at(0)
                lineSeries.replace(point.x, point.y, point.x, startElevation)
            }
        }
        Loader {
            id: wpLoader
            z: 1
            active: true
            anchors.fill: parent
            asynchronous: true
            sourceComponent: Component { WaypointsChart { } }
        }
        Loader {
            id: epLoader
            active: false
            anchors.fill: parent
            asynchronous: true
            sourceComponent: Component { ElevationChart { } }
        }
    }
    
    ColumnLayout {
        id: zoomLayout
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 2
        z: 5

        property real btnSize: 28

        IconButton {
            visible: true
            iconName: "fullscreen"
            size: zoomLayout.btnSize
            onTriggered: resetChartScale()
        }
        IconButton {
            visible: true
            iconName: "magnify-plus"
            size: zoomLayout.btnSize
            onTriggered: zoomAt(zoomFactor)
        }
        IconButton {
            visible: true
            iconName: "magnify-minus"
            size: zoomLayout.btnSize
            onTriggered: zoomAt(1 / zoomFactor)
        }
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

