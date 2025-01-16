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

// import Apx.Common
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

        ChartView {
            id: chartView
            property int margin: 5
            property var distance: mission.wp.distance

            anchors.fill: parent
            margins.top: margin
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
                // gridLineColor: "#40FFFFFF" 
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
            property var elevation: elevationmap.getElevationByCoordinate(coordinate)
            property var chartHeight: chartView.plotArea.height
            property var scaleY: axisY.max/chartHeight
            property var hStartPoint: !isNaN(elevation)?(elevation/scaleY):0

            x: chartView.plotArea.x
            y: chartView.plotArea.y + chartHeight

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
                    text: qsTr("S")
                    font.pixelSize: 12
                    font.bold: true
                }
            }
            Component.onCompleted: updateStartPoint()
            onCoordinateChanged: updateStartPoint()
            function updateStartPoint()
            {
                var point = lineSeries.at(0)
                lineSeries.replace(point.x, point.y, point.x, elevation)
            }
        }

        // Waypoints
        Repeater {
            id: repeater
            model: mission.wp.mapModel
            property int lastIndex: 0

            Item { 
                id: wpItem
                required property var modelData
                required property var index

                property var oldDistance: -1
                property var oldHAMSL: -1
                property var distance: modelData.totalDistance
                property var hAMSL: modelData.child("altitude").value // Calc for different cases
                property var coordinate: modelData.coordinate
                property var chartWidth: chartView.plotArea.width
                property var chartHeight: chartView.plotArea.height
                property var scaleX: axisX.max/chartWidth
                property var scaleY: axisY.max/chartHeight
                
                x: chartView.plotArea.x + distance/scaleX
                y: chartView.plotArea.y + chartHeight - hAMSL/scaleY
                Rectangle {
                    id: verticalLine
                    height: hAMSL/scaleY
                    width: 1
                    x: -width/2
                    y: 0
                    color: "#7FFFFFFF"
                }
                Rectangle {
                    id: chartPoint
                    height: 16
                    width: height
                    x: -width/2
                    y: -height/2
                    radius: height/8
                    color: "yellow"
                    Text {
                        anchors.centerIn: parent
                        text: modelData.num + 1
                        font.pixelSize: 12
                        font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: modelData.trigger()
                    }
                }
                Component.onCompleted: appendData()
                Component.onDestruction: removeData()
                onDistanceChanged: updateData()
                onHAMSLChanged: updateData()
                onCoordinateChanged: updateData() 

                function appendData() {
                    if(index>repeater.lastIndex)
                        lineSeries.append(distance,hAMSL)
                    else 
                        lineSeries.insert(1,distance,hAMSL)
                    repeater.lastIndex = index
                    setOldValues()
                }
                function updateData() {
                    lineSeries.replace(oldDistance, oldHAMSL, distance, hAMSL)
                    setOldValues()
                }
                function removeData() {
                    lineSeries.remove(oldDistance, oldHAMSL)
                }
                function setOldValues() {
                    oldDistance = distance
                    oldHAMSL = hAMSL 
                }
            }
        }
    }
}

