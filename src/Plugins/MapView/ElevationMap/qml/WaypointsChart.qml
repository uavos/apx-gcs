import QtQuick
import QtCharts
import QtQuick.Controls
import QtQuick.Window
import QtQml

import QtQml.Models

import APX.Fleet as APX
import APX.Mission

// Waypoint markers of the chart. The flight altitude line is drawn by
// ElevationView.missionLineShape from the same distances/heights.
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
            property bool amsl: modelData.child("amsl") ? modelData.child("amsl").value : false
            property var startHmsl: Math.round(mission.startElevation)
            property var altitude: modelData.child("altitude") ? modelData.child("altitude").value : 0
            property var agl: modelData.child("agl") ? modelData.child("agl").value : 0
            property var elevation: modelData.elevation
            property var unsafeAgl: modelData.unsafeAgl
            property bool collision: modelData.collision
            property bool alarmOn: !isNaN(elevation) ? (agl < unsafeAgl || collision) : false
            property var hAMSL: amsl ? altitude : altitude + startHmsl
            property var segmentLength: modelData ? modelData.distance : 0
            // position along the mission incl. runway part (synchronous, see ElevationView.segmentStarts)
            property var distance: modelData ? elevationView.segmentStart(index, modelData.totalDistanceWithRw - segmentLength) + segmentLength : -1
            property var num: modelData.num
            property var chartHeight: chartView.plotArea.height
            property var scaleY: axisY.max/chartHeight
            property var totalX: chartItem.x + x
            property var totalY: chartItem.y + y
            
            visible: distance >= 0
            x: elevationView.xOf(distance) // follows the zoomed/panned axis range
            y: chartView.plotArea.y + chartHeight - hAMSL/scaleY

            onSegmentLengthChanged: elevationView.scheduleSegmentStarts()
            onHAMSLChanged: elevationView.scheduleSegmentStarts()

            Rectangle {
                id: verticalLine
                height: wpItem.hAMSL/wpItem.scaleY
                width: 1
                x: -width/2
                y: 0
                color: "#7fffffff"
                visible: wpItem.getInViewArea(wpItem.totalX, wpItem.totalY, width, height)
            }
            Rectangle {
                id: chartPoint
                height: 18
                width: Math.max(height, pointText.contentWidth + 4)
                x: -width/2
                y: -height/2
                radius: height/8
                color: wpItem.alarmOn ? "#ffdead" : "#ffff00"
                border.color: wpItem.alarmOn ? "#ff0000" : "#40000000"
                border.width: 1
                visible: wpItem.getInViewArea(wpItem.totalX + x, wpItem.totalY + y, width, height)
                
                Text {
                    id: pointText
                    anchors.centerIn: parent
                    text: wpItem.num + 1
                    color: wpItem.alarmOn ? "#ff0000" : "#000000"
                    font.pixelSize: 12
                    font.bold: true
                }
                MouseArea {
                    id: ma
                    property bool moved: false
                    property var altitudeFact: modelData.child("altitude")
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SizeVerCursor
                    // vertical drag changes the waypoint altitude (fact only, no upload)
                    onPressed: (mouse) => {
                        moved = false
                        elevationView.waypointDragging = true
                    }
                    onPositionChanged: (mouse) => {
                        if(!pressed || !altitudeFact)
                            return
                        moved = true
                        var p = mapToItem(chartItem, mouse.x, mouse.y)
                        var hAMSL = (chartView.plotArea.y + wpItem.chartHeight - p.y) * wpItem.scaleY
                        var alt = wpItem.amsl ? hAMSL : hAMSL - wpItem.startHmsl
                        altitudeFact.value = Math.max(0, Math.round(alt))
                    }
                    onReleased: {
                        elevationView.waypointDragging = false
                        if(!moved)
                            modelData.trigger()
                    }
                    onCanceled: elevationView.waypointDragging = false
                }
            }

            function getInViewArea(totalX, totalY, width, height) {
                return (totalY <= elevationView.height && totalY + height >= 0) && (totalX <= elevationView.width && totalX + width >= 0)
            }
        }
    }
}
