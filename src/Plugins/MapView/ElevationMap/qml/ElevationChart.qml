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
        id: epItem

        required property var modelData
        required property var index

        property var fact: modelData
        property var num: fact ? fact.num : -1
        property var dist: fact ? fact.distance  : -1
        property var totalDistance: fact ? fact.totalDistanceWithRw : -1
        property var distance: num == 0 ? totalDistance : dist
        property var terrainProfile: fact ? fact.terrainProfile : null
        property var collision: fact ? fact.collision : false
        property var maxWidth: Screen.desktopAvailableWidth - 50
        property alias chartVisible: elevationProfile.visible
  
        onTerrainProfileChanged: epRect.updateLineSeriesData()
        onDistChanged: elevationProfile.visible = false

        // TerrainProfile 
        Rectangle {
            id: epRect
            property var scaleX: axisX.max/chartView.plotArea.width
            property var scaleY: axisY.max/chartView.plotArea.height
            property var totalX: chartItem.x + x
            property var totalY: chartItem.y + y
            property bool inViewArea: (totalY <= elevationView.height && totalY + height >= 0) && (totalX <= elevationView.width && totalX + width >= 0)
            visible: (totalDistance >=0 && distance >=0) && (x>=0 && y>=0) && inViewArea
            height: chartView.plotArea.height
            width: distance/scaleX
            x:  chartView.plotArea.x + (totalDistance - distance)/scaleX
            y:  chartView.plotArea.y
            color: "transparent"

            ChartView {
                id: elevationProfile
                anchors.fill: parent
                anchors.margins: -10
                anchors.leftMargin: -11 // -1 is the bias for bad positioning

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
                    color: epItem.collision ? "#40ff0000" : "#4000ff00"
                    borderColor: "transparent"
                    upperSeries: LineSeries {
                        id: epLineSeries
                    }
                }
                LineSeries {
                    id: epTopBorder
                    axisX: epAxisX
                    axisY: epAxisY
                    color: epItem.collision ? "#ff0000" : "#00ff00" 
                }
            }
            function updateLineSeriesData() {
                if(!terrainProfile)
                    return;
                if(distance >0 && !terrainProfile.length)
                    return;
                if(epLineSeries.count > 0) {
                    epTopBorder.removePoints(0, epTopBorder.count)
                    epLineSeries.removePoints(0, epLineSeries.count)
                }
                var groupDistance = mission.wp.distance
                if(groupDistance == 0 && totalDistance == 0) // for single point
                   return;
                var partWidth = distance * maxWidth / Math.max(groupDistance, totalDistance)
                var step = partWidth != 0 ? Math.round(2*terrainProfile.length / partWidth) : 1
                step = step > 0 ? step : 1
                for (var i = 0; i < terrainProfile.length; ++i) { 
                    if((i%step) == 0 || i == terrainProfile.length-1) {
                        epLineSeries.append(terrainProfile[i].x, terrainProfile[i].y)
                        epTopBorder.append(terrainProfile[i].x, terrainProfile[i].y)
                    }
                }
                elevationProfile.visible = true     
            }
        }
        Item {
            id: loading
            property var chartWidth: chartView.plotArea.width
            property var chartHeight: chartView.plotArea.height
            property var scaleX: axisX.max/chartWidth

            height: 3
            visible: !epItem.chartVisible && epRect.inViewArea
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
                width: epItem.distance/loading.scaleX
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
