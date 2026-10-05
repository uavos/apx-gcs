import QtQuick
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

        property var fact: modelData
        property int num: fact ? fact.num : -1
        property real dist: fact ? fact.distance : -1
        // segment start along the mission, m (synchronous, see ElevationView.segmentStarts).
        // Looked up by the waypoint number, not by the model row: the map model keeps
        // the creation order when a waypoint is inserted or reordered.
        // The chart starts at the first waypoint: its own leg (the takeoff) is not shown
        property real segmentStart: elevationView.segmentStart(num, 0)
        property real offset: segmentStart
        property real distance: dist

        onDistChanged: elevationView.scheduleSegmentStarts()
        onNumChanged: elevationView.scheduleSegmentStarts()

        // plot area of the shared chart
        x: chartView.plotArea.x
        y: chartView.plotArea.y
        width: chartView.plotArea.width
        height: chartView.plotArea.height
        clip: true

        missionItem: fact
        elevationMap: elevationmap
        xOffset: offset
        segmentLength: distance
        viewStart: elevationView.viewStart
        viewSpan: elevationView.viewSpan
        minHeight: axisY.min
        maxHeight: axisY.max
        fillColor: collision ? "#40ff0000" : "#4000ff00"
        lineColor: collision ? "#ff0000" : "#00ff00"
        lineWidth: 1.5
        visible: num > 0 && distance > 0
    }
}
