import QtQuick
import QtCharts
import QtQuick.Controls
import QtQuick.Window
import QtQml

import QtQml.Models

import APX.Fleet as APX
import APX.Mission

Repeater {
    id: repeater
    model: mission.wp.mapModel
    delegate: Item {
        required property var modelData
        required property var index

        z: ma.containsMouse ? 1 : 0

        // Waypoints
        Item { 
            id: wpItem
            property bool created: false
            property bool amsl: modelData.child("amsl").value
            property var startHmsl: mission.startElevation
            property var startPoint: mission.startPoint
            property var altitude: modelData.child("altitude").value
            property var agl: modelData.child("agl").value
            property var elevation: modelData.elevation
            property var unsafeAgl: modelData.unsafeAgl
            property bool collision: modelData.collision
            property bool alarmOn: !isNaN(elevation) ? (agl < unsafeAgl || collision) : false
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
                height: 18
                width: height
                x: -width/2
                y: -height/2
                radius: height/8
                color: wpItem.alarmOn ? "#ffdead" : "yellow"
                border.color: wpItem.alarmOn ? "red" : "#40000000"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: wpItem.num + 1
                    color: wpItem.alarmOn ? "red" : "black"
                    font.pixelSize: 12
                    font.bold: true
                }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
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
                    return
                }
                for(var i = 0; i < lineSeries.count; i++) {
                    var point = lineSeries.at(i)
                    if(point.x > distance) {
                        lineSeries.insert(i, distance, hAMSL)
                        setOldValues()
                        created = true
                        return
                    }
                }
                lineSeries.append(distance, hAMSL)
                setOldValues()
                created = true
            }
            function updateData() {
                if(!created)
                    return
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
