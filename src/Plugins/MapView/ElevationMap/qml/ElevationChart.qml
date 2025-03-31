import QtQuick
import QtCharts
import QtQuick.Controls
import QtQuick.Window
import QtQml
import QtQuick.Layouts

import QtQml.Models

import APX.Fleet as APX
import APX.Mission

Repeater {
    id: repeater
    model: mission.wp.mapModel
    delegate: Item {
        required property var modelData
        required property var index

        // TerrainProfile 
        Rectangle {
            id: epItem
            property var fact: modelData 
            property var scaleX: axisX.max/chartView.plotArea.width
            property var scaleY: axisY.max/chartView.plotArea.height
            property var terrainProfile: fact ? fact.terrainProfile : null
            property var totalDistance: fact ? fact.totalDistance : -1
            property var distance: fact ? fact.distance : -1
            property var collision: fact ? fact.collision : false
            property var maxWidth: Screen.desktopAvailableWidth - 50
            property alias chartVisible: elevationProfile.visible

            visible: totalDistance >=0 && distance >=0 && x>=0 && y>=0
            height: chartView.plotArea.height
            width: distance/scaleX
            x:  chartView.plotArea.x + (totalDistance -  distance)/scaleX
            y:  chartView.plotArea.y
            color: "transparent"

            ChartView {
                id: elevationProfile
                anchors.fill: parent
                plotArea: Qt.rect(x, y, width, height)
                backgroundColor: "transparent"
                legend.visible: false
                antialiasing: true
                
                margins.top: 0
                margins.bottom: 0
                margins.left: 0
                margins.right: 0

                ValueAxis {
                    id: epAxisX
                    min: 0
                    max: epItem.distance
                    lineVisible: false
                    labelsVisible: false
                    gridVisible: false
                }
                ValueAxis {
                    id: epAxisY
                    min: axisY.min
                    max: axisY.max
                    lineVisible: false
                    labelsVisible: false
                    gridVisible: false
                }
                AreaSeries {
                    id: areaSeries
                    axisX: epAxisX
                    axisY: epAxisY
                    color: epItem.collision ? "#FF0000" : "#00FF00"
                    borderColor: epItem.collision ? "#FF0000" : "#00FF00"
                    opacity: 0.25
                    upperSeries: LineSeries {
                        id: epLineSeries
                    }
                }
            }
            onTerrainProfileChanged: updateLineSeriesData()
            onDistanceChanged: elevationProfile.visible = false
            function updateLineSeriesData() {
                if(!terrainProfile)
                    return;
                //if(terrainProfile.lenght == 0)
                //    return; 
                if(distance >0 && !terrainProfile.length)
                    return;
                if(epLineSeries.count > 0)
                    epLineSeries.removePoints(0, epLineSeries.count)
                var groupDistance = mission.wp.distance
                var partWidth = distance * maxWidth / Math.max(groupDistance, totalDistance)
                var step = Math.round(2*terrainProfile.length / partWidth)
                step = step > 0 ? step : 1
                for (var i = 0; i < terrainProfile.length; ++i) 
                    if((i%step) == 0 || i == terrainProfile.length-1)
                        epLineSeries.append(terrainProfile[i].x, terrainProfile[i].y)
                elevationProfile.visible = true     
            }
        }
        Item {
            id: loading
            property var totalDistance: modelData.totalDistance
            property var distance: modelData.distance
            property var chartWidth: chartView.plotArea.width
            property var chartHeight: chartView.plotArea.height
            property var scaleX: axisX.max/chartWidth

            height: 3
            visible: !epItem.chartVisible
            y: chartView.plotArea.y + chartHeight

            onScaleXChanged: updateX()
            onVisibleChanged: if(!busyTimer.running) busyTimer.restart()
                
            Timer {
                id: busyTimer
                interval: 10000
                running: loading.visible
            }
            BusyIndicator {
                id: busy
                anchors.centerIn: parent
                running: busyTimer.running
                height: 32
                width:  32
            }
            Rectangle {
                height: parent.height
                width: loading.distance/loading.scaleX
                anchors.centerIn: parent
                visible: !busyTimer.running
                color: Material.accent
            }
            function updateX() {
                var newX = chartView.plotArea.x + (totalDistance-distance*0.5)/scaleX
                x = Math.abs(newX - x) > 1 ? newX : x
            }
        }
    }
}
