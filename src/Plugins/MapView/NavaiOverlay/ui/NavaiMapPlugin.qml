import QtQuick
import QtPositioning

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

            anchors.fill: parent
            visible: navai && navai.udpReady
            z: 100000

            Component.onCompleted: {
                console.log("NavaiMapPlugin loaded", navaiLayer.navai)
            }

            Connections {
                target: navaiLayer.baseMap

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

            Repeater {
                model: navaiLayer.navai
                    ? navaiLayer.navai.resultsModel
                    : null

                delegate: Item {
                    id: resultItem

                    property var centerCoord: QtPositioning.coordinate(
                        latitude,
                        longitude
                    )

                    property var edgeCoord: centerCoord.atDistanceAndAzimuth(
                        radiusMeters,
                        90
                    )

                    property var centerPoint: {
                        navaiLayer.mapRevision

                        if (!navaiLayer.baseMap)
                            return Qt.point(0, 0)

                        return navaiLayer.baseMap.fromCoordinate(
                            centerCoord,
                            false
                        )
                    }

                    property var edgePoint: {
                        navaiLayer.mapRevision

                        if (!navaiLayer.baseMap)
                            return Qt.point(0, 0)

                        return navaiLayer.baseMap.fromCoordinate(
                            edgeCoord,
                            false
                        )
                    }

                    property real rawPixelRadius: Math.sqrt(
                        Math.pow(edgePoint.x - centerPoint.x, 2) +
                        Math.pow(edgePoint.y - centerPoint.y, 2)
                    )

                    property real pixelRadius: Math.max(
                        rawPixelRadius,
                        18
                    )

                    x: centerPoint.x - pixelRadius
                    y: centerPoint.y - pixelRadius

                    width: pixelRadius * 2
                    height: pixelRadius * 2

                    opacity: itemOpacity
                    visible: itemOpacity > 0.01

                    z: 100000

                    Rectangle {
                        id: circle

                        anchors.fill: parent

                        radius: width / 2

                        color: Qt.rgba(
                            0.0,
                            0.25,
                            1.0,
                            0.25
                        )

                        border.color: Qt.rgba(
                            0.0,
                            0.55,
                            1.0,
                            1.0
                        )

                        border.width: 4
                    }

                    Rectangle {
                        id: centerDot

                        width: 8
                        height: 8

                        radius: 4

                        anchors.centerIn: parent

                        color: Qt.rgba(
                            0.0,
                            0.65,
                            1.0,
                            1.0
                        )
                    }

                    Rectangle {
                        id: labelBox

                        x: resultItem.pixelRadius - width / 2
                        y: -height - 8

                        width: labelText.implicitWidth + 14
                        height: labelText.implicitHeight + 8

                        radius: 5

                        color: Qt.rgba(
                            0.0,
                            0.08,
                            0.30,
                            0.90
                        )

                        border.color: Qt.rgba(
                            0.0,
                            0.55,
                            1.0,
                            1.0
                        )

                        border.width: 1

                        Text {
                            id: labelText

                            anchors.centerIn: parent

                            text: label
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
