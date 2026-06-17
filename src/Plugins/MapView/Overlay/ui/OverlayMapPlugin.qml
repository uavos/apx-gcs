import QtQuick
import QtLocation
import QtPositioning

import Apx.Application

AppPlugin {
    id: plugin

    sourceComponent: Component {
        Item {
            id: overlayRoot

            objectName: "TransparentMapOverlay"

            property var baseMap: ui.map
            property var overlayTool: apx.tools.overlay

            anchors.fill: parent
            visible: overlayTool && overlayTool.overlayTileServerReady
            z: 100000

            Plugin {
                id: overlayPlugin

                name: "osm"

                PluginParameter {
                    name: "osm.mapping.providersrepository.disabled"
                    value: true
                }

                PluginParameter {
                    name: "osm.mapping.custom.host"
                    value: overlayRoot.overlayTool
                        ? overlayRoot.overlayTool.overlayTileUrlTemplate
                        : "http://127.0.0.1:9292/tile/%z/%x/%y.png"
                }
            }

            Map {
                id: overlayMap

                anchors.fill: parent
                plugin: overlayPlugin

                visible: overlayRoot.visible
                enabled: false
                color: "transparent"

                center: overlayRoot.baseMap
                    ? overlayRoot.baseMap.center
                    : QtPositioning.coordinate(0, 0)

                zoomLevel: overlayRoot.baseMap
                    ? overlayRoot.baseMap.zoomLevel
                    : 16

                bearing: overlayRoot.baseMap
                    ? overlayRoot.baseMap.bearing
                    : 0

                tilt: overlayRoot.baseMap
                    ? overlayRoot.baseMap.tilt
                    : 0

                copyrightsVisible: false

                Component.onCompleted: {
                    selectCustomMapType()
                }

                function selectCustomMapType()
                {
                    for (var i = 0; i < supportedMapTypes.length; i++) {
                        var name = supportedMapTypes[i].name.toLowerCase()

                        if (name.indexOf("custom") >= 0) {
                            activeMapType = supportedMapTypes[i]
                            return
                        }
                    }
                }

                Timer {
                    interval: 1000
                    repeat: true
                    running: true

                    onTriggered: {
                        if (overlayMap.clearData)
                            overlayMap.clearData()
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
