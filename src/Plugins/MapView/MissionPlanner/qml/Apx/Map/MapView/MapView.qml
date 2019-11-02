import QtQuick          2.12
import QtLocation       5.12

import QtQuick.Controls 2.5
import QtGraphicalEffects 1.0

import Apx.Map.Vehicles 1.0
import Apx.Map.Mission 1.0
import Apx.Map.Navigation 1.0

Control {
    id: control

    property bool showVehicles: true
    property bool showMission: showVehicles
    property bool showNavigation: showVehicles

    property bool showWind: showNavigation
    property bool showScale: showNavigation
    property bool showInfo: showNavigation


    property alias map: map

    signal mapBackgroundItemLoaded(var item)

    //internal
    property var mapPlugin: apx.tools.missionplanner

    background: Item {
        id: mapTilesItem
        anchors.fill: parent
        layer.enabled: ui.effects
        layer.effect: ShaderEffect {
            fragmentShader: Qt.resolvedUrl("/shaders/vignette.fsh")
        }

        function connectOverlay(mapBase)
        {
            mapBase.z=map.z - 100000
            map.minimumFieldOfView=mapBase.minimumFieldOfView
            map.maximumFieldOfView=mapBase.maximumFieldOfView
            map.minimumTilt=mapBase.minimumTilt
            map.maximumTilt=mapBase.maximumTilt
            map.minimumZoomLevel=mapBase.minimumZoomLevel
            map.maximumZoomLevel=mapBase.maximumZoomLevel

            mapBase.center=Qt.binding(function(){return map.center})
            mapBase.zoomLevel=Qt.binding(function(){return map.zoomLevel})
            mapBase.tilt=Qt.binding(function(){return map.tilt})
            mapBase.bearing=Qt.binding(function(){return map.bearing})
        }

        Loader {
            id: mapLoader
            anchors.fill: parent
            asynchronous: false
            sourceComponent: mapBaseC
            onLoaded: {
                mapTilesItem.connectOverlay(item)
                control.mapBackgroundItemLoaded(item)
            }
            //reload map item on provider change
            property string provider: mapPlugin.prefs.provider.text.trim()
            onProviderChanged: {
                mapLoader.active=false
                mapLoader.active=true
            }
        }

        Component {
            id: mapBaseC
            Map {
                id: mapBase
                gesture.enabled: false
                focus: false

                color: "#333"
                copyrightsVisible: false

                Component.onCompleted: {
                    var vtypes=[]
                    for(var i in mapBase.supportedMapTypes){
                        var m=mapBase.supportedMapTypes[i]
                        vtypes.push(m.description)
                    }
                    mapPlugin.prefs.maptype.enumStrings=vtypes
                    //mapPlugin.prefs.maptype.value=activeMapType.description
                }
                property string mapTypeName: mapPlugin.prefs.maptype.text
                activeMapType: {
                    var v = mapTypeName
                    for(var i in mapBase.supportedMapTypes){
                        var m=mapBase.supportedMapTypes[i]
                        if(m.description !== v) continue
                        return m
                    }
                    return mapBase.supportedMapTypes[0]
                }

                plugin: Plugin {
                    allowExperimental: true
                    preferred: [
                        mapLoader.provider,
                        "osm"
                    ]
                    Component.onCompleted: {
                        mapPlugin.prefs.provider.enumStrings=availableServiceProviders
                    }
                }

                layer.enabled: ui.effects
                layer.effect: FastBlur {
                    id: mapBlur
                    anchors.fill: mapBase
                    source: mapBase
                    radius: ui.antialiasing?4:0
                    //deviation: 1
                    enabled: ui.antialiasing
                }
            }
        }

        MapItemsOverlay {
            id: map
            anchors.fill: parent

            color: "transparent"
            plugin: Plugin { name: "itemsoverlay" }

            Loader {
                active: showVehicles
                asynchronous: true
                sourceComponent: VehiclesMapItems { }
            }
            Loader {
                active: showMission
                asynchronous: true
                sourceComponent: MissionMapItems { }
            }
            Loader {
                active: showNavigation
                asynchronous: true
                sourceComponent: NavigationMapItems { }
            }

            Connections {
                enabled: showVehicles && showNavigation
                target: apx.vehicles.current.mission
                onTriggered: map.showRegion(apx.vehicles.current.mission.boundingGeoRectangle())
            }
        }
    }
}
