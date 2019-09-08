import QtQuick          2.3
import QtLocation       5.13
import QtPositioning    5.3
import QtQuick.Window   2.2
import QtQuick.Layouts 1.3

import QtQml 2.12
import QtGraphicalEffects 1.0

import APX.Vehicles     1.0

import Apx.Common 1.0
import Apx.Controls 1.0

import "./vehicle"
import "./mission"
import "./lib"

Item {
    id: control

    property bool showVehicles: false
    property bool showVehicleNav: showVehicles
    property bool showWind: showVehicleNav
    property bool showScale: showVehicleNav

    Item {
        id: mapControlsItem
        anchors.fill: parent
        layer.enabled: ui.effects
        //layer.smooth: ui.antialiasing
        layer.effect: ShaderEffect {
            fragmentShader: Qt.resolvedUrl("/shaders/vignette.fsh")
        }

        Component.onCompleted: {
            application.registerUiComponent(map,"map")
            ui.main.containerBottom.anchors.rightMargin=scaleLegend.width+scaleLegend.anchors.margins+ui.main.containerBottom.anchors.margins
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
            //mapBase.fieldOfView=Qt.binding(function(){return map.fieldOfView})
            //mapBase.scale=Qt.binding(function(){return map.scale})
        }

        Loader {
            id: mapLoader
            anchors.fill: parent
            asynchronous: false
            property bool locationPluginAvailable:
                apx.settings.application.plugins.location
                && apx.settings.application.plugins.location.value
                && (apx.tools.location?true:false)
            property string provider: {
                var s="" //default
                if(!mapLoader.locationPluginAvailable)return s
                var v=apx.tools.location.provider.text.trim()
                if(!v)return s
                return v
            }
            onLoaded: {
                mapControlsItem.connectOverlay(mapLoader.item)
                application.registerUiComponent(item,"mapbase")
            }
            onProviderChanged: {
                mapLoader.active=false
                mapLoader.active=true
            }
            sourceComponent: mapBaseC
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
                    if(apx.tools.location){
                        apx.tools.location.maptype.enumStrings=vtypes
                        apx.tools.location.maptype.value=activeMapType.description
                    }
                }
                property string mapTypeName: apx.tools.location?apx.tools.location.maptype.text:""
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
                        if(apx.tools.location){
                            apx.tools.location.provider.enumStrings=availableServiceProviders
                        }
                    }
                }

                //visible: false
                layer.enabled: ui.effects
                //layer.smooth: ui.antialiasing
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

            /*plugin: Plugin {
                name: "mapboxgl"

                PluginParameter {
                    name: "mapboxgl.mapping.items.insert_before"
                    value: "road-label-small"
                }

                PluginParameter {
                    name: "mapboxgl.access_token"
                    value: "pk.eyJ1IjoidG1wc2FudG9zIiwiYSI6ImNqMWVzZWthbDAwMGIyd3M3ZDR0aXl3cnkifQ.FNxMeWCZgmujeiHjl44G9Q"
                }

                PluginParameter {
                    name: "mapboxgl.mapping.additional_style_urls"
                    value: "mapbox://styles/mapbox/navigation-guidance-day-v2,mapbox://styles/mapbox/navigation-guidance-night-v2,mapbox://styles/mapbox/navigation-preview-day-v2,mapbox://styles/mapbox/navigation-preview-night-v2"
                }
            }
            activeMapType: {
                var v = "Satellite Streets"
                for(var i in map.supportedMapTypes){
                    var m=map.supportedMapTypes[i]
                    if(m.description !== v) continue
                    return m
                }
                return map.supportedMapTypes[0]
            }
            copyrightsVisible: false
            MapParameter {
                type: "layer"

                property var name: "3d-buildings"
                property var source: "composite"
                property var sourceLayer: "building"
                property var layerType: "fill-extrusion"
                property var minzoom: 15.0
            }

            MapParameter {
                type: "filter"

                property var layer: "3d-buildings"
                property var filter: [ "==", "extrude", "true" ]
            }

            MapParameter {
                type: "paint"

                property var layer: "3d-buildings"
                property var fillExtrusionColor: "#00617f"
                property var fillExtrusionOpacity: .6
                property var fillExtrusionHeight: { return { type: "identity", property: "height" } }
                property var fillExtrusionBase: { return { type: "identity", property: "min_height" } }
            }*/



















            //initial animation
            PropertyAnimation {
                running: true
                target: map
                property: "zoomLevel"
                from: 12
                to: 16.33
                duration: 1000
                easing.type: Easing.OutInCirc
            }

            //VEHICLES
            Loader {
                active: showVehicles
                asynchronous: true
                Component.onCompleted: setSource("vehicle/VehicleItem.qml",{"vehicle": apx.vehicles.LOCAL, "z": z})
                onLoaded: {
                    item.z=200
                    map.addMapItem(item)
                }
            }


            Loader {
                active: showVehicles
                asynchronous: true
                Component.onCompleted: setSource("vehicle/VehicleItem.qml",{"vehicle": apx.vehicles.REPLAY, "z": z})
                onLoaded: {
                    item.z=201
                    map.addMapItem(item)
                }
            }

            Loader {
                active: showVehicles
                asynchronous: true
                Component.onCompleted: setSource("vehicle/VehiclesMapItems.qml")
                onLoaded: {
                    item.z=202
                    map.addMapItemView(item)
                }
            }

            //Current vehicle items
            Loader {
                active: showVehicleNav
                asynchronous: true
                sourceComponent: Component { EnergyCircle { } }
                onLoaded: map.addMapItem(item)
            }
            Loader {
                active: showVehicleNav
                asynchronous: true
                sourceComponent: Component { CmdPosCircle { } }
                onLoaded: map.addMapItem(item)
            }
            Loader {
                active: showVehicleNav
                asynchronous: true
                sourceComponent: Component { Home { } }
                onLoaded: map.addMapItem(item)
            }
            Loader {
                active: showVehicleNav
                asynchronous: true
                sourceComponent: Component { LoiterCircle { } }
                onLoaded: map.addMapItem(item)
            }

            Loader { //travel path from telemetry
                active: showVehicleNav && apx.vehicles.current.isReplay()
                asynchronous: true
                source: "vehicle/PathTelemetry.qml"
                onLoaded: map.addMapItem(item)
            }

            Loader { //travel path current
                active: showVehicleNav && !apx.vehicles.current.isReplay()
                asynchronous: true
                source: "vehicle/PathCurrent.qml"
                onLoaded: map.addMapItem(item)
            }


            Connections {
                target: apx.vehicles.current.mission
                onTriggered: map.showRegion(apx.vehicles.current.mission.boundingGeoRectangle())
            }

        }
    }

    //Controls
    Loader {
        id: windItem
        active: showWind
        asynchronous: true
        sourceComponent: Component { Wind { } }
        anchors.bottom: scaleLegend.top
        anchors.right: parent.right
        anchors.margins: 10*ui.scale
    }
    Loader {
        id: scaleLegend
        active: showScale
        asynchronous: true
        sourceComponent: Component { MapScale { } }
        width: 100*ui.scale
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 20*ui.scale
    }
    Loader {
        id: mapBusy
        active: showScale
        asynchronous: true
        sourceComponent: Component { MapBusy { } }
        z: scaleLegend.z
        anchors.left: scaleLegend.left
        anchors.right: scaleLegend.right
        anchors.top: scaleLegend.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 2*ui.scale
        anchors.bottomMargin: 4*ui.scale
    }

}
