import QtQuick
import QtQuick.Controls
import QtQml

import QtQml.Models

import APX.Fleet as APX
import APX.Mission
import Apx.Elevation 1.0

// Terrain profile segments of the mission, one scene-graph item per waypoint.
// Each item covers the chart plot area and maps distance/elevation to pixels itself
// from the axis ranges, so zoom and pan cost only property updates (no raster repaint).
Repeater {
    id: repeater
    model: mission.wp.mapModel
    delegate: TerrainProfileItem {
        id: epItem

        required property var modelData
        required property var index

        property var fact: modelData
        property int num: fact ? fact.num : -1
        property real dist: fact ? fact.distance : -1
        property real totalDistance: fact ? fact.totalDistanceWithRw : -1
        property real distance: num == 0 ? totalDistance : dist
        // segment start along the mission, m (synchronous, see ElevationView.segmentStarts)
        property real offset: elevationView.segmentStart(index, totalDistance - distance)
        property bool collision: fact ? fact.collision : false

        onDistChanged: elevationView.scheduleSegmentStarts()
        property bool hasProfile: pointCount > 1 && distance > 0 && totalDistance >= 0

        // plot area of the shared chart
        x: chartView.plotArea.x
        y: chartView.plotArea.y
        width: chartView.plotArea.width
        height: chartView.plotArea.height
        clip: true

        missionItem: fact
        xOffset: offset
        segmentLength: distance
        viewStart: elevationView.viewStart
        viewSpan: elevationView.viewSpan
        minHeight: axisY.min
        maxHeight: axisY.max
        fillColor: collision ? "#40ff0000" : "#4000ff00"
        lineColor: collision ? "#ff0000" : "#00ff00"
        lineWidth: 1.5
        visible: hasProfile

        // Busy/placeholder bar while the profile is not available.
        // Lives in the plot layer (not a child of the clipped profile item).
        Item {
            id: loading
            parent: epItem.parent
            property real xStart: elevationView.xOf(epItem.offset)
            property real xEnd: elevationView.xOf(epItem.offset + epItem.distance)
            property bool inViewArea: xEnd >= chartView.plotArea.x
                                      && xStart <= chartView.plotArea.x + chartView.plotArea.width

            height: 3
            width: Math.max(xEnd - xStart, 1)
            x: xStart
            y: chartView.plotArea.y + chartView.plotArea.height
            visible: !epItem.hasProfile && epItem.distance > 0 && inViewArea

            onVisibleChanged: if(!busyTimer.running) busyTimer.restart()

            Timer {
                id: busyTimer
                interval: 10000
                running: loading.visible
            }
            BusyIndicator {
                id: busy
                anchors.centerIn: parent
                running: busyTimer.running && loading.visible
                height: 32
                width:  32
            }
            Rectangle {
                anchors.fill: parent
                visible: !busyTimer.running
                color: Material.accent
            }
        }
    }
}
