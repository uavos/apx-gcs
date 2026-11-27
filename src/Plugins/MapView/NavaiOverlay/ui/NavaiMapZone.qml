import QtQuick
import QtLocation
import QtPositioning

import Apx.Map.Common

// The zone of a map to build on board: a polygon edited like a geofence,
// with the tile count and size written under its lower right corner.
MapItemGroup {
    id: zone

    property var plan
    property var map: ui.map // MapObject looks the map up by this name

    readonly property var path: plan ? plan.polygon.perimeter : []
    readonly property bool exists: path.length >= 3
    readonly property color color: "#2962FF"

    // the vertex new points are added next to
    property int selectedPointIndex: -1

    z: 40

    // Middle of the side before or after the selected vertex.
    function sideCenter(prev)
    {
        var n = path.length
        var i = selectedPointIndex
        if (i < 0 || i >= n || n < 2)
            return QtPositioning.coordinate()
        var a = path[i]
        var b = path[prev ? (i > 0 ? i - 1 : n - 1) : (i < n - 1 ? i + 1 : 0)]
        return a.atDistanceAndAzimuth(a.distanceTo(b) / 2, a.azimuthTo(b))
    }

    // "New zone" puts a rectangle in the middle of the visible map.
    Connections {
        target: zone.plan
        function onZoneRequested()
        {
            var w = zone.map.width
            var h = zone.map.height
            zone.plan.setZone([
                zone.map.toCoordinate(Qt.point(w * 0.3, h * 0.3), false),
                zone.map.toCoordinate(Qt.point(w * 0.7, h * 0.3), false),
                zone.map.toCoordinate(Qt.point(w * 0.7, h * 0.7), false),
                zone.map.toCoordinate(Qt.point(w * 0.3, h * 0.7), false)
            ])
        }
    }

    MapPolygon {
        visible: zone.exists
        opacity: 0.8
        // a faint wash, so the tiles and the map stay readable under it
        color: Qt.alpha(Qt.lighter(zone.color, 1.9), 0.12)
        border.width: 4
        border.color: zone.color
        path: zone.path
    }

    // zone vertices: select to drag, press and hold to remove
    MapItemView {
        id: points
        property bool selectNew: false
        model: zone.plan ? zone.plan.pointsModel : 0
        delegate: MapObject {
            radiusFactor: 2
            color: zone.color
            title: (hover || selected) ? modelData.num + 1 : ""
            opacity: ui.effects ? 0.8 : 1
            implicitCoordinate: modelData.coordinate
            onMoved: modelData.coordinate = coordinate
            onSelectedChanged: {
                if (selected)
                    zone.selectedPointIndex = modelData.num
                else if (zone.selectedPointIndex === modelData.num)
                    zone.selectedPointIndex = -1
            }
            Component.onCompleted: {
                if (points.selectNew) {
                    points.selectNew = false
                    select()
                }
            }
            onPressAndHold: {
                if (zone.path.length > 3) {
                    var n = modelData.num
                    deselect()
                    zone.plan.removePoint(n)
                }
            }
        }
    }

    // two extra points next to the selected vertex to add a new one
    Repeater {
        model: 2
        delegate: MapObject {
            implicitZ: -1
            readonly property bool prev: model.index === 0
            radiusFactor: 2
            color: zone.color
            title: "+"
            opacity: ui.effects ? 0.8 : 1
            visible: zone.exists && zone.selectedPointIndex >= 0
            implicitCoordinate: zone.sideCenter(prev)
            // a click adds the point, the handle itself is not selected or dragged
            interactive: false
            MouseArea {
                cursorShape: Qt.PointingHandCursor
                anchors.fill: parent
                onClicked: {
                    points.selectNew = true
                    zone.plan.addPoint(parent.coordinate,
                                       parent.prev ? zone.selectedPointIndex
                                                   : zone.selectedPointIndex + 1)
                }
            }
        }
    }

    // what the zone costs: tiles per scale and the size on the board
    MapQuickItem {
        visible: zone.exists && summary.text !== ""
        coordinate: zone.plan ? zone.plan.labelCoordinate : QtPositioning.coordinate()
        anchorPoint.x: label.width
        anchorPoint.y: -6
        sourceItem: Rectangle {
            id: label
            width: summary.implicitWidth + 12
            height: summary.implicitHeight + 8
            radius: 3
            color: Qt.rgba(0.0, 0.08, 0.30, 0.8)
            border.color: zone.color
            border.width: 1
            Text {
                id: summary
                anchors.centerIn: parent
                text: zone.plan ? zone.plan.summary : ""
                textFormat: Text.StyledText
                color: "white"
                font: apx.font_narrow(zone.map.fontSize)
            }
        }
    }
}
