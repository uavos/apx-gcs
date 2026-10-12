import QtQuick
import QtLocation
import QtPositioning

import Apx.Common

// ATS overlay items for a single GCS unit
MapItemGroup {
    id: item

    readonly property var unit: modelData
    readonly property var ats: unit.mandala.est.ats

    readonly property real yaw: ats.yaw.value
    readonly property real pitch: ats.pitch.value

    // beam heading: exponential smoothing suppresses telemetry jitter
    readonly property real yawFilter: 0.5 // 0..1, lower is smoother
    property real beamYaw: 0
    Component.onCompleted: beamYaw = yaw
    onYawChanged: {
        var d = (yaw - beamYaw) % 360
        if (d > 180)
            d -= 360
        else if (d < -180)
            d += 360
        beamYaw = (beamYaw + yawFilter * d + 360) % 360
    }

    readonly property var origin: unit.coordinate
    readonly property bool valid: unit.isGroundControl && unit.coordinate.isValid

    readonly property real beamDistance: overlay.beamDistance
    readonly property real tickHalf: beamDistance * 0.006 // m, major tick half length
    readonly property bool showBeam: valid && overlay.showBeam

    readonly property color beamColor: "#cc00ffff"

    visible: showBeam

    function point(c, bearing, distance) {
        return c.atDistanceAndAzimuth(distance, bearing)
    }

    // BEAM

    MapPolygon {
        visible: item.showBeam
        z: 50
        color: "#20ffff00"
        border.width: 2
        border.color: "#80ffff00"
        path: {
            if (!item.valid)
                return []
            var pts = [item.origin]
            var steps = 32
            var a0 = item.beamYaw - overlay.beamHalfAngle
            var da = 2 * overlay.beamHalfAngle / steps
            for (var i = 0; i <= steps; i++)
                pts.push(item.point(item.origin, a0 + da * i, item.beamDistance))
            return pts
        }
    }

    MapPolyline {
        visible: item.showBeam
        z: 51
        line.width: 2
        line.color: item.beamColor
        path: item.valid ? [item.origin, item.point(item.origin, item.beamYaw, item.beamDistance)] : []
    }

    // ruler ticks along the beam
    Repeater {
        model: 10
        MapPolyline {
            readonly property bool major: (index + 1) % 2 === 0
            readonly property real dist: item.beamDistance * (index + 1) / 10
            readonly property real half: major ? item.tickHalf : item.tickHalf / 2
            readonly property var center: item.point(item.origin, item.beamYaw, dist)

            visible: item.showBeam
            z: 51
            line.width: major ? 2 : 1
            line.color: item.beamColor
            path: item.valid ? [item.point(center, item.beamYaw - 90, half), item.point(center, item.beamYaw + 90, half)] : []
        }
    }

    // ruler labels at major ticks
    Repeater {
        model: 5
        MapQuickItem {
            readonly property real dist: item.beamDistance * (index + 1) / 5
            readonly property real side: (item.beamYaw + 90) * Math.PI / 180 // label side of the tick
            // keep the label clear of the tick: gap plus the label extent along the tick direction
            readonly property real offset: Style.fontSize * 0.3
                                           + Math.abs(Math.sin(side)) * sourceItem.width / 2
                                           + Math.abs(Math.cos(side)) * sourceItem.height / 2

            visible: item.showBeam
            z: 51
            coordinate: item.valid
                        ? item.point(item.point(item.origin, item.beamYaw, dist), item.beamYaw + 90, item.tickHalf)
                        : QtPositioning.coordinate()
            anchorPoint.x: sourceItem.width / 2 - offset * Math.sin(side)
            anchorPoint.y: sourceItem.height / 2 + offset * Math.cos(side)
            sourceItem: Text {
                text: Math.round(dist / 1000)
                color: "#00ffff"
                font: apx.font_narrow(Style.fontSize * 0.8, true)
            }
        }
    }

    // azimuth and elevation near the antenna
    MapQuickItem {
        visible: item.showBeam
        z: 52
        coordinate: item.valid ? item.origin : QtPositioning.coordinate()
        anchorPoint.x: sourceItem.width + Style.fontSize * 0.5
        anchorPoint.y: -Style.fontSize * 0.5
        sourceItem: Text {
            text: "AZ " + item.yaw.toFixed(0) + "° EL " + item.pitch.toFixed(1) + "°"
            color: "#00ffff"
            font: apx.font_narrow(Style.fontSize, true)
        }
    }
}
