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

    flags: Qt.WindowStaysOnTopHint
    width: Screen.desktopAvailableWidth - 50
    height: 200
    minimumHeight: 200
    minimumWidth: 600
    title: qsTr("Terrain elevation")
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
    }
    Label {
        id: axisXLabel
        anchors.bottom: parent.bottom
        width: parent.width
        text: qsTr("Distance, %1").arg("m")
        horizontalAlignment: Text.AlignHCenter
    }
    Item {
        id: chartItem
        height: elevationView.height
        width: elevationView.width

        Rectangle {
            id: alarm
            property int margin: 5

            height: txt.height
            width: txt.width + margin
            color: "red"
            radius: 2
            border.width: radius
            border.color: "white"
            visible: false
            anchors {
                top: parent.top
                left: parent.left
                topMargin: margin
                leftMargin: margin
            }

            Text {
                id: txt
                text: "Alarm"
                color: "white"
                font.bold: true
                font.pixelSize: Style.fontSize*0.8
                anchors.centerIn: parent
            }
        }

        ChartView {
            id: chartView
            property int margin: 5
            property var distance: mission.wp.distance

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
                max: chartView.distance!=0?chartView.distance:10
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
                min: 0
                max: 2000
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
                XYPoint{x:0; y:0}
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
            Component.onCompleted: updateStartPoint()
            onCoordinateChanged: updateStartPoint()
            onStartElevationChanged: updateStartPoint()
            function updateStartPoint()
            {
                if(hStartPoint === undefined)
                    return;
                var point = lineSeries.at(0)
               lineSeries.replace(point.x, point.y, point.x, hStartPoint)
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

