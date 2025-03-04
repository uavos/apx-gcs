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

Repeater {
    id: repeater
    model: mission.wp.mapModel
    property int lastIndex: 0
    delegate: Item {
        required property var modelData
        required property var index

        // TerrainProfile 
        Rectangle {
            id: epItem
            property var scaleX: axisX.max/chartView.plotArea.width
            property var scaleY: axisY.max/chartView.plotArea.height

            height: chartView.plotArea.height
            width: modelData.distance/scaleX
            x:  chartView.plotArea.x + (modelData.totalDistance -  modelData.distance)/scaleX
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
                    max: modelData.distance
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
                    color: "#00FF00"
                    borderColor: "#00FF00"
                    opacity: 0.25
                    upperSeries: LineSeries {
                        id: epLineSeries
                    }

                }
            }
            Component.onCompleted: setLineSeriesData()
            function setLineSeriesData() {
                var elevationProfile =  startPoint.elevationmap.getElevationProfile(modelData.geoPath)
                elevationProfile.forEach((ep)=>{epLineSeries.append(ep.x, ep.y)})
            }
        }
    }
}
