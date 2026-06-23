import QtQuick
import QtLocation
import QtPositioning

MapItemGroup {
    id: beamGroup

    property var showBeamFact: apx.tools.ats.overlay.show_beam
    property bool showBeam: showBeamFact ? showBeamFact.value : false

    property var showCompassFact: apx.tools.ats.overlay.show_compass
    property bool showCompass: showCompassFact ? showCompassFact.value : false

    property var compassRadiusFact: apx.tools.ats.overlay.compass_radius

    property var beamDistanceFact: apx.tools.ats.overlay.beam_distance

    readonly property real beamDistance: (beamDistanceFact ? beamDistanceFact.value : 30) * 1000
    readonly property real circleRadius: (compassRadiusFact ? compassRadiusFact.value : 5) * 1000
    readonly property real halfAngle: 2

    function destinationPoint(coord, bearing, distance) {
        return coord.atDistanceAndAzimuth(distance, bearing)
    }

    function getGcsYaw(unit) {
        if (!unit || !unit.mandala) return 0
        return unit.mandala.est.att.yaw.value
    }

    // Get reference to the map
    property var map: parent

    //BEAM
    Instantiator {
        id: gcsInstantiator
        model: apx.fleet.model
        active: beamGroup.showBeam

        delegate: QtObject {
            id: gcsDelegate
            property var unit: modelData
            property bool isGcs: unit ? unit.isGroundControl : false
            property bool valid: isGcs && unit.coordinate.isValid && beamGroup.showBeam

            property var cone: null
            property var centerLine: null

            onValidChanged: {
                if (valid && !cone) createItems()
                if (!valid && cone) destroyItems()
            }
            Component.onCompleted: if (valid) createItems()
            Component.onDestruction: destroyItems()

            function createItems() {
                if (!beamGroup.map) return
                cone = coneComp.createObject(beamGroup.map, {})
                if (cone) beamGroup.map.addMapItem(cone)
                centerLine = lineComp.createObject(beamGroup.map, {})
                if (centerLine) beamGroup.map.addMapItem(centerLine)
            }

            function destroyItems() {
                if (cone) { cone.destroy(); cone = null }
                if (centerLine) { centerLine.destroy(); centerLine = null }
            }

            property var updateTimer: Timer {
                interval: 100
                repeat: true
                running: gcsDelegate.valid
                onTriggered: gcsDelegate.updateAll()
            }

            function updateAll() {
                if (!unit || !unit.coordinate.isValid) return
                var origin = unit.coordinate
                var yaw = beamGroup.getGcsYaw(unit)

                if (cone) {
                    var coords = [origin]
                    var steps = 32
                    for (var i = 0; i <= steps; i++) {
                        var a = (yaw - beamGroup.halfAngle) + (2.0 * beamGroup.halfAngle) * (i / steps)
                        coords.push(beamGroup.destinationPoint(origin, a, beamGroup.beamDistance))
                    }
                    cone.path = coords
                }

                if (centerLine) {
                    var endPt = beamGroup.destinationPoint(origin, yaw, beamGroup.beamDistance)
                    if (centerLine.pathLength() === 0) {
                        centerLine.addCoordinate(origin)
                        centerLine.addCoordinate(endPt)
                    } else {
                        centerLine.replaceCoordinate(0, origin)
                        centerLine.replaceCoordinate(1, endPt)
                    }
                }
            }
        }
    }

    Component {
        id: coneComp
        MapPolygon {
            color: "#20ffff00"
            border.width: 2
            border.color: "#80ffff00"
            z: 50
        }
    }
    Component {
        id: lineComp
        MapPolyline {
            line.width: 2
            line.color: "#cc00ffff"
            z: 51
        }
    }

    // Ruler ticks
    Repeater {
        model: 10
        MapPolyline {
            id: rulerTick
            visible: beamGroup.showBeam && apx.fleet.gcs !== null && apx.fleet.gcs.coordinate.isValid
            line.width: (index + 1) % 2 === 0 ? 2 : 1
            line.color: "#cc00ffff"
            z: 51
            property real dist: beamGroup.beamDistance * (index + 1) / 10
            property real tickHalf: (index + 1) % 2 === 0 ? 150 : 75
            function updatePath() {
                if (!apx.fleet.gcs || !apx.fleet.gcs.coordinate.isValid) return
                var origin = apx.fleet.gcs.coordinate
                var yaw = beamGroup.getGcsYaw(apx.fleet.gcs)
                var center = beamGroup.destinationPoint(origin, yaw, dist)
                var p1 = beamGroup.destinationPoint(center, yaw - 90, tickHalf)
                var p2 = beamGroup.destinationPoint(center, yaw + 90, tickHalf)
                while (rulerTick.pathLength() > 0) rulerTick.removeCoordinate(0)
                rulerTick.addCoordinate(p1)
                rulerTick.addCoordinate(p2)
            }
            Timer { interval: 100; repeat: true; running: rulerTick.visible; onTriggered: rulerTick.updatePath() }
            Component.onCompleted: updatePath()
        }
    }

    // Ruler labels
    Repeater {
        model: 5
        MapQuickItem {
            visible: beamGroup.showBeam && apx.fleet.gcs !== null && apx.fleet.gcs.coordinate.isValid
            property real dist: beamGroup.beamDistance * (index + 1) * 2 / 10
            coordinate: apx.fleet.gcs
                        ? beamGroup.destinationPoint(
                              beamGroup.destinationPoint(apx.fleet.gcs.coordinate, beamGroup.getGcsYaw(apx.fleet.gcs), dist),
                              beamGroup.getGcsYaw(apx.fleet.gcs) + 90, 250)
                        : QtPositioning.coordinate()
            anchorPoint.x: 0; anchorPoint.y: distLabel.height
            z: 51
            sourceItem: Text { id: distLabel; text: Math.round(dist/1000)+""; color: "#cc00ffff"; font.pixelSize: 11; font.bold: true }
        }
    }

    // COMPASS
    Instantiator {
        id: compassInstantiator
        model: apx.fleet.model
        active: beamGroup.showCompass

        delegate: QtObject {
            id: compassDelegate
            property var unit: modelData
            property bool isGcs: unit ? unit.isGroundControl : false
            property bool valid: isGcs && unit.coordinate.isValid && beamGroup.showCompass

            property var circle: null

            onValidChanged: {
                if (valid && !circle) createCircle()
                if (!valid && circle) destroyCircle()
            }
            Component.onCompleted: if (valid) createCircle()
            Component.onDestruction: destroyCircle()

            function createCircle() {
                if (!beamGroup.map) return
                circle = circleComp.createObject(beamGroup.map, {})
                if (circle) beamGroup.map.addMapItem(circle)
            }

            function destroyCircle() {
                if (circle) { circle.destroy(); circle = null }
            }

            property var updateTimer: Timer {
                interval: 100
                repeat: true
                running: compassDelegate.valid
                onTriggered: {
                    if (!unit || !unit.coordinate.isValid) return
                    if (circle) circle.center = unit.coordinate
                }
            }
        }
    }

    Component {
        id: circleComp
        MapCircle {
            radius: beamGroup.circleRadius
            color: "transparent"
            border.width: 3
            border.color: "#60ffffff"
            z: 49
        }
    }

    // Major ticks
    Repeater {
        model: 12
        MapPolyline {
            id: majorTick
            visible: beamGroup.showCompass && apx.fleet.gcs !== null && apx.fleet.gcs.coordinate.isValid
            line.width: index % 3 === 0 ? 3 : 2
            line.color: index % 3 === 0 ? "#aaffffff" : "#60ffffff"
            z: 49
            property real bearing: index * 30
            function updatePath() {
                if (!apx.fleet.gcs || !apx.fleet.gcs.coordinate.isValid) return
                var origin = apx.fleet.gcs.coordinate
                var p1 = beamGroup.destinationPoint(origin, bearing, beamGroup.circleRadius * 0.85)
                var p2 = beamGroup.destinationPoint(origin, bearing, beamGroup.circleRadius)
                if (majorTick.pathLength() === 0) { majorTick.addCoordinate(p1); majorTick.addCoordinate(p2) }
                else { majorTick.replaceCoordinate(0, p1); majorTick.replaceCoordinate(1, p2) }
            }
            Timer { interval: 100; repeat: true; running: majorTick.visible; onTriggered: majorTick.updatePath() }
            Component.onCompleted: updatePath()
        }
    }

    // Minor ticks
    Repeater {
        model: 36
        MapPolyline {
            id: minorTick
            visible: (index % 3 !== 0) && beamGroup.showCompass && apx.fleet.gcs !== null && apx.fleet.gcs.coordinate.isValid
            line.width: 1.5
            line.color: "#80ffffff"
            z: 49
            property real bearing: index * 10
            function updatePath() {
                if (!apx.fleet.gcs || !apx.fleet.gcs.coordinate.isValid) return
                var origin = apx.fleet.gcs.coordinate
                var p1 = beamGroup.destinationPoint(origin, bearing, beamGroup.circleRadius * 0.93)
                var p2 = beamGroup.destinationPoint(origin, bearing, beamGroup.circleRadius)
                if (minorTick.pathLength() === 0) { minorTick.addCoordinate(p1); minorTick.addCoordinate(p2) }
                else { minorTick.replaceCoordinate(0, p1); minorTick.replaceCoordinate(1, p2) }
            }
            Timer { interval: 100; repeat: true; running: minorTick.visible; onTriggered: minorTick.updatePath() }
            Component.onCompleted: updatePath()
        }
    }

    // Degree labels
    Repeater {
        model: 12
        MapQuickItem {
            visible: beamGroup.showCompass && apx.fleet.gcs !== null && apx.fleet.gcs.coordinate.isValid
            coordinate: apx.fleet.gcs
                        ? beamGroup.destinationPoint(apx.fleet.gcs.coordinate, index * 30, beamGroup.circleRadius * 1.12)
                        : QtPositioning.coordinate()
            anchorPoint.x: label.width / 2
            anchorPoint.y: label.height / 2
            z: 49
            sourceItem: Text {
                id: label
                text: { var d = index*30; return d===0?"N":d===90?"E":d===180?"S":d===270?"W":d+"°" }
                color: (index % 3 === 0) ? "#ffffff" : "#aaaaaa"
                font.pixelSize: (index % 3 === 0) ? 16 : 13
                font.bold: index % 3 === 0
            }
        }
    }
}
