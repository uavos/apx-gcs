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
        // Waypoints
        Item { 
            id: wpItem
            property bool created: false
            property bool amsl: modelData.child("amsl").value
            property var startHmsl: mission.startElevation
            property var startPoint: mission.startPoint
            property var altitude: modelData.child("altitude").value
            property var agl: modelData.child("agl").value
            property bool alarmOn: agl < modelData.unsafeAgl || modelData.collision
            property var hAMSL: amsl ? altitude : altitude + startHmsl
            property var distance: modelData ? modelData.totalDistance : -1
            property var coordinate: modelData.coordinate
            property var num: modelData.num
            property var chartWidth: chartView.plotArea.width
            property var chartHeight: chartView.plotArea.height
            property var scaleX: axisX.max/chartWidth
            property var scaleY: axisY.max/chartHeight
            property var oldDistance: -1
            property var oldHAMSL: -1
            
            visible: distance > 0 || created 
            x: chartView.plotArea.x + distance/scaleX
            y: chartView.plotArea.y + chartHeight - hAMSL/scaleY
            Rectangle {
                id: verticalLine
                height: wpItem.hAMSL/wpItem.scaleY
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
                color: wpItem.alarmOn ? "#ffdead" : "yellow"
                border.color: wpItem.alarmOn ? "red" : "transparent"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: wpItem.num + 1
                    color: wpItem.alarmOn ? "red" : "black"
                    font.pixelSize: 12
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: modelData.trigger()
                }
            }
            Timer {
                id: timer
                interval: 100
                onTriggered: wpItem.appendData()
            }
            Component.onCompleted: timer.start()
            Component.onDestruction: removeData()
            onVisibleChanged: if(visible) timer.start()
            onDistanceChanged: updateData()
            onHAMSLChanged: updateData()
            onCoordinateChanged: updateData() 

            function appendData() {
                if(!visible) {
                    if(num != 0)
                        return
                    if(!startPoint.isValid) {
                        created = true
                        return
                    }
                }
                var dst = distance > 0 ? distance : 0.1 //fix for incorrect line drawing at zero values
                if(index>repeater.lastIndex)
                    lineSeries.append(dst,hAMSL)
                else
                    lineSeries.insert(1,dst,hAMSL)
                repeater.lastIndex = index
                setOldValues()
                created = true
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
