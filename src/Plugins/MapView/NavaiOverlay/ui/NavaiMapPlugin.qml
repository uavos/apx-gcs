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
            property var navai: apx.tools.navai
            property int mapRevision: 0
            property real trajectoryLineWidth: 2
            property real trajectoryEndpointDiameter: 8
            // Labels and uncertainty circles only become useful when the
            // operator is close enough to inspect an individual result.
            property real resultDetailsMinZoom: 16
            // 250 m cells are readable from roughly 5 km map scale, but are
            // completely hidden when zooming out farther.
            property real tileGridMinZoom: 8

            property bool navaiAvailable: navai !== null && navai !== undefined
            property bool navaiActive: navaiAvailable && navai.active
            property bool navaiUdpReady: navaiAvailable && navai.udpReady
            property bool navaiHasHeatmap: navaiAvailable &&
                navai.heatmapTiles.length > 0

            anchors.fill: parent
            visible: navaiAvailable && navaiActive
            z: 100000

            Component.onCompleted: {
                console.log("NavaiMapPlugin loaded", navaiLayer.navai)
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

            Plugin {
                id: overlayPlugin
                name: "osm"
                PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }
                PluginParameter {
                    name: "osm.mapping.custom.host"
                    value: navaiLayer.navai
                        ? navaiLayer.navai.overlayTileUrlTemplate
                        : "http://127.0.0.1:9293/navai-grid-v2/tile/%z/%x/%y.png"
                }
            }

            Map {
                id: overlayMap
                anchors.fill: parent
                plugin: overlayPlugin
                enabled: false
                color: "transparent"
                opacity: 1.0
                visible: navaiLayer.navaiActive && navaiLayer.navaiUdpReady &&
                         navaiLayer.navaiHasHeatmap &&
                         navaiLayer.navai.overlayTileServerReady &&
                         navaiLayer.baseMap &&
                         navaiLayer.baseMap.zoomLevel >= navaiLayer.tileGridMinZoom
                center: navaiLayer.baseMap ? navaiLayer.baseMap.center : QtPositioning.coordinate(0, 0)
                zoomLevel: navaiLayer.baseMap ? navaiLayer.baseMap.zoomLevel : 16
                bearing: navaiLayer.baseMap ? navaiLayer.baseMap.bearing : 0
                tilt: navaiLayer.baseMap ? navaiLayer.baseMap.tilt : 0
                copyrightsVisible: false

                Component.onCompleted: selectCustomMapType()
                function selectCustomMapType() {
                    for (var i = 0; i < supportedMapTypes.length; ++i) {
                        if (supportedMapTypes[i].name.toLowerCase().indexOf("custom") >= 0) {
                            activeMapType = supportedMapTypes[i]
                            return
                        }
                    }
                }
            }

            Repeater {
                model: navaiLayer.navaiActive && navaiLayer.navaiUdpReady &&
                       navaiLayer.baseMap &&
                       navaiLayer.baseMap.zoomLevel >= navaiLayer.tileGridMinZoom
                    ? navaiLayer.navai.heatmapTiles
                    : []

                delegate: Item {
                    id: heatItem
                    required property var modelData
                    property var attachedMap: null

                    function attach() {
                        if (attachedMap === navaiLayer.baseMap)
                            return
                        if (attachedMap)
                            attachedMap.removeMapItem(heatPolygon)
                        attachedMap = navaiLayer.baseMap
                        if (attachedMap)
                            attachedMap.addMapItem(heatPolygon)
                    }

                    Component.onCompleted: Qt.callLater(attach)
                    Component.onDestruction: {
                        if (attachedMap)
                            attachedMap.removeMapItem(heatPolygon)
                    }

                    MapPolygon {
                        id: heatPolygon
                        path: heatItem.modelData.polygon
                        color: Qt.rgba(
                            Qt.color(heatItem.modelData.color).r,
                            Qt.color(heatItem.modelData.color).g,
                            Qt.color(heatItem.modelData.color).b,
                            0.02
                        )
                        border.width: 1
                        border.color: heatItem.modelData.color
                        z: 99901 + heatItem.modelData.score
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
                    required property real tileLatitude
                    required property real tileLongitude
                    required property real radiusMeters
                    required property real percent
                    required property string label
                    required property real itemOpacity
                    required property var trajectoryCoordinates
                    required property bool latest

                    property var trajectoryMap: null
                    property bool detailsVisible: latest ||
                        (navaiLayer.baseMap &&
                         navaiLayer.baseMap.zoomLevel >=
                             navaiLayer.resultDetailsMinZoom)

                    function attachTrajectory()
                    {
                        if (trajectoryMap === navaiLayer.baseMap)
                            return

                        if (trajectoryMap) {
                            trajectoryMap.removeMapItem(trajectoryLine)
                            trajectoryMap.removeMapItem(trajectoryEndpoint)
                        }

                        trajectoryMap = navaiLayer.baseMap

                        if (trajectoryMap) {
                            trajectoryMap.addMapItem(trajectoryLine)
                            trajectoryMap.addMapItem(trajectoryEndpoint)
                        }
                    }

                    Component.onCompleted: Qt.callLater(attachTrajectory)
                    Component.onDestruction: {
                        if (trajectoryMap) {
                            trajectoryMap.removeMapItem(trajectoryLine)
                            trajectoryMap.removeMapItem(trajectoryEndpoint)
                        }
                    }

                    Connections {
                        target: navaiLayer

                        function onBaseMapChanged() {
                            Qt.callLater(resultItem.attachTrajectory)
                        }
                    }

                    MapPolyline {
                        id: trajectoryLine

                        visible: resultItem.trajectoryCoordinates.length > 1
                        opacity: resultItem.itemOpacity
                        z: 99999
                        line.width: navaiLayer.trajectoryLineWidth
                        line.color: "#ff7a00"
                        path: resultItem.trajectoryCoordinates
                    }

                    MapQuickItem {
                        id: trajectoryEndpoint

                        visible: resultItem.trajectoryCoordinates.length > 0
                        opacity: resultItem.itemOpacity
                        z: 100000
                        anchorPoint.x: endpointDot.width / 2
                        anchorPoint.y: endpointDot.height / 2

                        coordinate: visible
                            ? resultItem.trajectoryCoordinates[
                                resultItem.trajectoryCoordinates.length - 1
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

                    property bool validCoordinate:
                        isFinite(latitude) &&
                        isFinite(longitude) &&
                        latitude >= -90 &&
                        latitude <= 90 &&
                        longitude >= -180 &&
                        longitude <= 180

                    property bool validTileArea:
                        isFinite(tileLatitude) &&
                        isFinite(tileLongitude) &&
                        tileLatitude >= -90 &&
                        tileLatitude <= 90 &&
                        tileLongitude >= -180 &&
                        tileLongitude <= 180 &&
                        isFinite(radiusMeters) &&
                        radiusMeters > 0

                    property var centerCoord: validCoordinate
                        ? QtPositioning.coordinate(latitude, longitude)
                        : QtPositioning.coordinate(0, 0)

                    property var tileCenterCoord: validTileArea
                        ? QtPositioning.coordinate(tileLatitude, tileLongitude)
                        : QtPositioning.coordinate(0, 0)

                    property var edgeCoord: validTileArea
                        ? tileCenterCoord.atDistanceAndAzimuth(radiusMeters, 90)
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

                        if (!validTileArea || !navaiLayer.baseMap)
                            return Qt.point(0, 0)

                        var p = navaiLayer.baseMap.fromCoordinate(
                            edgeCoord,
                            false
                        )

                        if (!p)
                            return Qt.point(0, 0)

                        return p
                    }

                    property var tileCenterPoint: {
                        navaiLayer.mapRevision

                        if (!validTileArea || !navaiLayer.baseMap)
                            return Qt.point(0, 0)

                        var p = navaiLayer.baseMap.fromCoordinate(
                            tileCenterCoord,
                            false
                        )

                        return p ? p : Qt.point(0, 0)
                    }

                    property real rawPixelRadius: radiusMeters > 0
                        ? Math.sqrt(
                            Math.pow(edgePoint.x - tileCenterPoint.x, 2) +
                            Math.pow(edgePoint.y - tileCenterPoint.y, 2)
                        )
                        : 0

                    property real pixelRadius: rawPixelRadius

                    x: 0
                    y: 0
                    width: navaiLayer.width
                    height: navaiLayer.height

                    opacity: itemOpacity
                    visible: validCoordinate && itemOpacity > 0.01

                    z: 100000

                    Rectangle {
                        id: circle

                        visible: resultItem.detailsVisible &&
                                 resultItem.validTileArea &&
                                 resultItem.pixelRadius > 0
                        x: resultItem.tileCenterPoint.x - width / 2
                        y: resultItem.tileCenterPoint.y - height / 2
                        width: resultItem.pixelRadius * 2
                        height: width
                        radius: width / 2

                        color: Qt.rgba(0.0, 0.25, 1.0, 0.25)

                        border.color: Qt.rgba(0.0, 0.55, 1.0, 1.0)
                        border.width: 4
                    }

                    Rectangle {
                        id: centerDot

                        width: resultItem.detailsVisible ? 16 : 8
                        height: width
                        radius: width / 2

                        x: resultItem.centerPoint.x - width / 2
                        y: resultItem.centerPoint.y - height / 2

                        color: resultItem.detailsVisible
                            ? Qt.rgba(0.0, 0.65, 1.0, 1.0)
                            : Qt.rgba(0.0, 0.40, 0.85, 0.75)
                        border.color: resultItem.detailsVisible
                            ? "white"
                            : "#168cff"
                        border.width: resultItem.detailsVisible ? 2 : 1
                        z: 2
                    }

                    Rectangle {
                        id: labelBox

                        visible: resultItem.detailsVisible

                        x: resultItem.centerPoint.x - width / 2
                        y: resultItem.centerPoint.y - height - 12

                        width: labelText.implicitWidth + 14
                        height: labelText.implicitHeight + 8

                        radius: 5

                        color: Qt.rgba(0.0, 0.08, 0.30, 0.90)

                        border.color: Qt.rgba(0.0, 0.55, 1.0, 1.0)
                        border.width: 1
                        z: 3

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
    }

    Connections {
        target: application

        function onUiComponentLoaded(name, object) {
            updateMap()
        }
    }
}
