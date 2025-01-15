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

    width: Screen.desktopAvailableWidth - 50
    height: 200
    minimumHeight: 200
    minimumWidth: 600
    title: qsTr("Terrain elevation")
    color: "#CC000000"
    visible: true
    x: 25
    y: Screen.desktopAvailableHeight - height - 50

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
            property APX.Unit unit: apx.fleet.current
            readonly property Mission mission: unit.mission
            property var distance: mission.wp.distance
            readonly property bool empty: mission.empty

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
                max: (chartView.empty&&chartView.distance!=0)?10000:chartView.distance
                lineVisible: true
                labelsFont.family: axisXLabel.font.family
                labelsFont.pointSize: axisXLabel.font.pointSize
                labelsColor: axisXLabel.color
                gridLineColor: "#40FFFFFF" 
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
            }
            Repeater {
                id: repeater
                model: chartView.mission.wp.mapModel
                Item { 
                    required property var modelData
                    property var d: modelData.totalDistance
                    property var h: modelData.child("altitude").value
                    Component.onCompleted: {lineSeries.append(d,h)}
                }
            }
        }
    }
}

