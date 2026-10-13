import QtQuick
import QtPositioning
import QtLocation

import Apx.Application
import Apx.Navai 1.0

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
            property real opticalLineWidth: 3
            // Labels only become useful when the operator is close enough to
            // inspect an individual result.
            property real resultDetailsMinZoom: 16
            // 250 m cells are readable from roughly 5 km map scale, but are
            // completely hidden when zooming out farther.
            property real tileGridMinZoom: 8

            property bool navaiAvailable: navai !== null && navai !== undefined
            property bool navaiActive: navaiAvailable && navai.active
            property bool navaiUdpReady: navaiAvailable && navai.udpReady
            property bool navaiHasGrid: navaiAvailable && navai.tileCount > 0

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

            // The tile grid and the DINO scores, painted in one pass from the
            // map copy kept by the plugin, so a huge map costs no QML items.
            NavaiHeatmap {
                id: heatmap
                anchors.fill: parent
                visible: navaiLayer.navaiActive && navaiLayer.navaiUdpReady &&
                         navaiLayer.navaiHasGrid &&
                         navaiLayer.baseMap &&
                         navaiLayer.baseMap.zoomLevel >= navaiLayer.tileGridMinZoom
                navai: navaiLayer.navaiAvailable ? navaiLayer.navai : null
                center: navaiLayer.baseMap
                    ? navaiLayer.baseMap.center
                    : QtPositioning.coordinate(0, 0)
                zoomLevel: navaiLayer.baseMap ? navaiLayer.baseMap.zoomLevel : 0
                bearing: navaiLayer.baseMap ? navaiLayer.baseMap.bearing : 0
                z: 99900
            }

            // Optical flow track: where the camera says the aircraft went.
            Repeater {
                model: navaiLayer.navaiActive && navaiLayer.navaiUdpReady
                    ? navaiLayer.navai.opticalTrack
                    : []

                delegate: Item {
                    id: opticalItem
                    required property var modelData
                    property var attachedMap: null

                    function attach() {
                        if (attachedMap === navaiLayer.baseMap)
                            return
                        if (attachedMap)
                            attachedMap.removeMapItem(opticalLine)
                        attachedMap = navaiLayer.baseMap
                        if (attachedMap)
                            attachedMap.addMapItem(opticalLine)
                    }

                    Component.onCompleted: attach()
                    Component.onDestruction: {
                        if (attachedMap)
                            attachedMap.removeMapItem(opticalLine)
                    }

                    MapPolyline {
                        id: opticalLine
                        visible: opticalItem.modelData.length > 1
                        path: opticalItem.modelData
                        line.width: navaiLayer.opticalLineWidth
                        line.color: "#ffd600"
                        opacity: 0.6
                        z: 99990
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
                    required property real percent
                    required property real radiusMeters
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
                            trajectoryMap.removeMapItem(errorCircle)
                            trajectoryMap.removeMapItem(trajectoryLine)
                            trajectoryMap.removeMapItem(trajectoryEndpoint)
                        }

                        trajectoryMap = navaiLayer.baseMap

                        if (trajectoryMap) {
                            trajectoryMap.addMapItem(errorCircle)
                            trajectoryMap.addMapItem(trajectoryLine)
                            trajectoryMap.addMapItem(trajectoryEndpoint)
                        }
                    }

                    Component.onCompleted: attachTrajectory()
                    Component.onDestruction: {
                        if (trajectoryMap) {
                            trajectoryMap.removeMapItem(errorCircle)
                            trajectoryMap.removeMapItem(trajectoryLine)
                            trajectoryMap.removeMapItem(trajectoryEndpoint)
                        }
                    }

                    // Error radius of the position: the aircraft is inside this circle.
                    MapCircle {
                        id: errorCircle

                        visible: resultItem.validCoordinate && resultItem.radiusMeters > 0
                        opacity: resultItem.itemOpacity
                        z: 99995
                        center: resultItem.centerCoord
                        radius: resultItem.radiusMeters
                        color: Qt.rgba(0.0, 0.55, 1.0, 0.12)
                        border.color: Qt.rgba(0.0, 0.55, 1.0, 0.7)
                        border.width: 1
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

                    property var centerCoord: validCoordinate
                        ? QtPositioning.coordinate(latitude, longitude)
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

                    x: 0
                    y: 0
                    width: navaiLayer.width
                    height: navaiLayer.height

                    opacity: itemOpacity
                    visible: validCoordinate && itemOpacity > 0.01

                    z: 100000

                    Rectangle {
                        id: centerDot

                        width: resultItem.detailsVisible ? 10 : 6
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

                        width: labelText.implicitWidth + 8
                        height: labelText.implicitHeight + 4

                        radius: 3

                        color: Qt.rgba(0.0, 0.08, 0.30, 0.75)

                        border.color: Qt.rgba(0.0, 0.55, 1.0, 0.8)
                        border.width: 1
                        z: 3

                        Text {
                            id: labelText

                            anchors.centerIn: parent

                            text: resultItem.label
                            color: "white"

                            font.pixelSize: 10
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
