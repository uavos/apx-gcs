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
import QtQml

import QtQml.Models

import Apx.Common
// import Apx.Controls
// import Apx.Instruments
// import Apx.Application

import APX.Fleet as APX
import APX.Mission

Window {
    id: elevationView
    property APX.Unit unit: apx.fleet.current
    readonly property Mission mission: unit.mission
    readonly property bool empty: mission.empty
    property var elevationmap: apx.tools.elevationmap
    property var use: elevationmap ? elevationmap.use.value : false 
    property var plugin: apx.settings.application.plugins.elevationmap
    property var pluginOn: plugin ? plugin.value : false
    property var chartOn: use && pluginOn
    property var name: qsTr("Terrain elevation")
    property var disabled: qsTr("(disabled)")

    flags: Qt.WindowStaysOnTopHint
    width: Screen.desktopAvailableWidth - 50
    height: 200
    minimumHeight: 200
    minimumWidth: 600
    title: chartOn ? name : name + " " + disabled
    color: "#CC000000"
    visible: true
    x: 25
    y: Screen.desktopAvailableHeight - height - 50

    onClosing: plugin.active=false
    onVisibleChanged: timer.restart()

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
    Item {
        id: chartItem
        height: elevationView.height
        width: elevationView.width
        visible: elevationView.chartOn

        Rectangle {
            id: alarm
            property int margin: 5

            height: txt.height
            width: icon.width + txt.width + 2*margin
            color: "red"
            radius: 2
            border.width: radius
            border.color: "white"
            visible: mission.collision
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
                text: "Alarm"
                color: "white"
                font.bold: true
                font.pixelSize: Style.fontSize*0.8
                anchors.left: icon.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: alarm.margin/2
            }
            SequentialAnimation {
                running: true
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

        ChartView {
            id: chartView
            property int margin: 5
            property var distance: mission.wp.distance
            property var minHeight: mission.minHeight
            property var maxHeight: mission.maxHeight
            anchors.fill: parent
            margins.top: alarm.height
            margins.right: margin
            margins.bottom: margin
            margins.left: 2*margin
            backgroundColor: "transparent"
            legend.visible: false
            antialiasing: true

            ValueAxis {
                id: axisX
                min: 0
                max: Math.max(chartView.distance, 1000)
                lineVisible: true
                labelsFont.family: axisXLabel.font.family
                labelsFont.pointSize: axisXLabel.font.pointSize
                labelsColor: axisXLabel.color
                gridVisible: false
                tickCount: 11
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
                gridLineColor: "#40FFFFFF"
                tickCount: 5
                labelFormat: "%.0f"
            }
            LineSeries {
                id: lineSeries
                axisX: axisX
                axisY: axisY
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

            visible: mission.startPoint.isValid
            x: chartView.plotArea.x
            y: chartView.plotArea.y + chartHeight
            z: 1

            Rectangle {
                id: takeOffPoint
                height: 16
                width: height
                x: -width/2
                y: -height/2 - startPoint.hStartPoint
                radius: height/8
                color: "white"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("R")
                    font.pixelSize: 12
                    font.bold: true
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
}

