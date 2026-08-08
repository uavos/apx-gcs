import QtQuick
import QtPositioning
import QtLocation

import Apx.Application

AppPlugin {
    id: plugin

    sourceComponent: Component {
        Item {
            id: navaiLayer

            objectName: "NavaiResultsOverlay"

            property var baseMap: ui.map
            property var attachedMap: null
            property var historicalTrajectoryItems: []
            property var navai: apx.tools.navai
            property int mapRevision: 0
            property real trajectoryLineWidth: 4
            property real trajectoryEndpointDiameter: 8

            property bool navaiAvailable: navai !== null && navai !== undefined
            property bool navaiActive: navaiAvailable && navai.active
            property bool navaiUdpReady: navaiAvailable && navai.udpReady

            anchors.fill: parent
            visible: navaiAvailable && navaiActive
            z: 100000

            function attachTrajectory()
            {
                if (attachedMap === baseMap)
                    return

                if (attachedMap) {
                    attachedMap.removeMapItem(matchedTrajectoryLine)
                    attachedMap.removeMapItem(matchedTrajectoryEndpoint)
                    clearHistoricalTrajectories()
                }

                attachedMap = baseMap

                if (attachedMap) {
                    attachedMap.addMapItem(matchedTrajectoryLine)
                    attachedMap.addMapItem(matchedTrajectoryEndpoint)
                    rebuildHistoricalTrajectories()
                }
            }

            function clearHistoricalTrajectories()
            {
                for (var i = 0; i < historicalTrajectoryItems.length; ++i) {
                    if (attachedMap)
                        attachedMap.removeMapItem(historicalTrajectoryItems[i])
                    historicalTrajectoryItems[i].destroy()
                }
                historicalTrajectoryItems = []
            }

            function rebuildHistoricalTrajectories()
            {
                clearHistoricalTrajectories()
                if (!attachedMap || !navaiAvailable)
                    return

                var paths = navai.historicalTrajectories
                var items = []
                for (var i = 0; i < paths.length; ++i) {
                    var item = historicalTrajectoryComponent.createObject(
                        navaiLayer,
                        { "path": paths[i] }
                    )
                    attachedMap.addMapItem(item)
                    items.push(item)

                    if (paths[i].length > 0) {
                        var endpoint = historicalTrajectoryEndpointComponent.createObject(
                            navaiLayer,
                            { "coordinate": paths[i][paths[i].length - 1] }
                        )
                        attachedMap.addMapItem(endpoint)
                        items.push(endpoint)
                    }
                }
                historicalTrajectoryItems = items
            }

            onBaseMapChanged: Qt.callLater(attachTrajectory)

            Component.onCompleted: {
                console.log("NavaiMapPlugin loaded", navaiLayer.navai)
                Qt.callLater(attachTrajectory)
            }

            Component.onDestruction: {
                clearHistoricalTrajectories()
                if (attachedMap)
                    attachedMap.removeMapItem(matchedTrajectoryLine)
                if (attachedMap)
                    attachedMap.removeMapItem(matchedTrajectoryEndpoint)
            }

            Connections {
                target: navaiLayer.navai
                ignoreUnknownSignals: true

                function onHistoricalTrajectoriesChanged() {
                    navaiLayer.rebuildHistoricalTrajectories()
                }
            }

            Connections {
                target: navaiLayer.baseMap
                ignoreUnknownSignals: true

                function onCenterChanged() {
                    navaiLayer.mapRevision++
                }

                function onZoomLevelChanged() {
                    navaiLayer.mapRevision++
                }

                function onBearingChanged() {
                    navaiLayer.mapRevision++
                }

                function onTiltChanged() {
                    navaiLayer.mapRevision++
                }

                function onWidthChanged() {
                    navaiLayer.mapRevision++
                }

                function onHeightChanged() {
                    navaiLayer.mapRevision++
                }
            }

            MapPolyline {
                id: matchedTrajectoryLine

                visible: navaiLayer.navaiAvailable &&
                         navaiLayer.navaiActive &&
                         navaiLayer.navai.matchedTrajectoryCoordinates.length > 1

                z: 99999
                line.width: navaiLayer.trajectoryLineWidth
                line.color: "#ff7a00"

                path: navaiLayer.navaiAvailable
                    ? navaiLayer.navai.matchedTrajectoryCoordinates
                    : []
            }

            MapQuickItem {
                id: matchedTrajectoryEndpoint

                visible: navaiLayer.navaiAvailable &&
                         navaiLayer.navaiActive &&
                         navaiLayer.navai.matchedTrajectoryCoordinates.length > 0

                z: 100000
                anchorPoint.x: endpointDot.width / 2
                anchorPoint.y: endpointDot.height / 2

                coordinate: visible
                    ? navaiLayer.navai.matchedTrajectoryCoordinates[
                        navaiLayer.navai.matchedTrajectoryCoordinates.length - 1
                      ]
                    : QtPositioning.coordinate(0, 0)

                sourceItem: Rectangle {
                    id: endpointDot
                    width: navaiLayer.trajectoryEndpointDiameter
                    height: width
                    radius: width / 2
                    color: "#ff7a00"
                }
            }

            Component {
                id: historicalTrajectoryComponent

                MapPolyline {
                    z: 99998
                    line.width: navaiLayer.trajectoryLineWidth
                    line.color: "#66ff7a00"
                }
            }

            Component {
                id: historicalTrajectoryEndpointComponent

                MapQuickItem {
                    z: 99999
                    anchorPoint.x: historicalEndpointDot.width / 2
                    anchorPoint.y: historicalEndpointDot.height / 2

                    sourceItem: Rectangle {
                        id: historicalEndpointDot
                        width: navaiLayer.trajectoryEndpointDiameter
                        height: width
                        radius: width / 2
                        color: "#66ff7a00"
                    }
                }
            }

            Repeater {
                model: navaiLayer.navaiActive && navaiLayer.navaiUdpReady
                    ? navaiLayer.navai.resultsModel
                    : 0

                delegate: Item {
                    id: resultItem

                    required property real latitude
                    required property real longitude
                    required property real radiusMeters
                    required property real percent
                    required property string label
                    required property real itemOpacity

                    property bool validCoordinate:
                        isFinite(latitude) &&
                        isFinite(longitude) &&
                        latitude >= -90 &&
                        latitude <= 90 &&
                        longitude >= -180 &&
                        longitude <= 180 &&
                        radiusMeters >= 0

                    property var centerCoord: validCoordinate
                        ? QtPositioning.coordinate(latitude, longitude)
                        : QtPositioning.coordinate(0, 0)

                    property var edgeCoord: validCoordinate && radiusMeters > 0
                        ? centerCoord.atDistanceAndAzimuth(radiusMeters, 90)
                        : QtPositioning.coordinate(0, 0)

                    property var centerPoint: {
                        navaiLayer.mapRevision

                        if (!validCoordinate || !navaiLayer.baseMap)
                            return Qt.point(0, 0)

                        var p = navaiLayer.baseMap.fromCoordinate(
                            centerCoord,
                            false
                        )

                        if (!p)
                            return Qt.point(0, 0)

                        return p
                    }

                    property var edgePoint: {
                        navaiLayer.mapRevision

                        if (!validCoordinate || !navaiLayer.baseMap)
                            return Qt.point(0, 0)

                        var p = navaiLayer.baseMap.fromCoordinate(
                            edgeCoord,
                            false
                        )

                        if (!p)
                            return Qt.point(0, 0)

                        return p
                    }

                    property real rawPixelRadius: radiusMeters > 0
                        ? Math.sqrt(
                            Math.pow(edgePoint.x - centerPoint.x, 2) +
                            Math.pow(edgePoint.y - centerPoint.y, 2)
                        )
                        : 0

                    property real pixelRadius: Math.max(
                        rawPixelRadius,
                        18
                    )

                    x: centerPoint.x - pixelRadius
                    y: centerPoint.y - pixelRadius

                    width: pixelRadius * 2
                    height: pixelRadius * 2

                    opacity: itemOpacity
                    visible: validCoordinate && itemOpacity > 0.01

                    z: 100000

                    Rectangle {
                        id: circle

                        anchors.fill: parent
                        radius: width / 2

                        color: Qt.rgba(0.0, 0.25, 1.0, 0.25)

                        border.color: Qt.rgba(0.0, 0.55, 1.0, 1.0)
                        border.width: 4
                    }

                    Rectangle {
                        id: centerDot

                        width: 16
                        height: 16
                        radius: 8

                        anchors.centerIn: parent

                        color: Qt.rgba(0.0, 0.65, 1.0, 1.0)
                    }

                    Rectangle {
                        id: labelBox

                        x: resultItem.pixelRadius - width / 2
                        y: -height - 8

                        width: labelText.implicitWidth + 14
                        height: labelText.implicitHeight + 8

                        radius: 5

                        color: Qt.rgba(0.0, 0.08, 0.30, 0.90)

                        border.color: Qt.rgba(0.0, 0.55, 1.0, 1.0)
                        border.width: 1

                        Text {
                            id: labelText

                            anchors.centerIn: parent

                            text: resultItem.label
                            color: "white"

                            font.pixelSize: 14
                            font.bold: true
                        }
                    }
                }
            }
        }
    }

    uiComponent: "map"

    onLoaded: updateMap()

    function updateMap()
    {
        if (!ui.map || !plugin.item)
            return

        plugin.item.parent = ui.map
        plugin.item.anchors.fill = ui.map
        plugin.item.z = 100000
        Qt.callLater(plugin.item.attachTrajectory)
    }

    Connections {
        target: application

        function onUiComponentLoaded(name, object) {
            updateMap()
        }
    }
}
