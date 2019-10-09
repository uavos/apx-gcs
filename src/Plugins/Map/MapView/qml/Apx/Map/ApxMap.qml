import QtQuick          2.3
import QtLocation       5.13
import QtPositioning    5.3
import QtQuick.Window   2.2
import QtQuick.Layouts 1.3

import QtQml 2.12
import QtGraphicalEffects 1.0

import APX.Facts        1.0
import APX.Vehicles     1.0

import Apx.Common 1.0
import Apx.Controls 1.0

import "./vehicle"
import "./mission"
import "./lib"

Item {
    id: control

    property bool showVehicles: true
    property bool showVehicleNav: showVehicles
    property bool showWind: showVehicleNav
    property bool showScale: showVehicleNav
    property bool showInfo: showVehicleNav

    property var mapPlugin: apx.tools.mapview

    Item {
        id: mapControlsItem
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
                mapControlsItem.connectOverlay(item)
                application.registerUiComponent(item,"mapbase")
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
                    mapPlugin.prefs.maptype.value=activeMapType.description
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

            Component.onCompleted: {
                application.registerUiComponent(map,"map")
                //ui.main.containerBottom.anchors.rightMargin=scaleLegend.width+scaleLegend.anchors.margins+ui.main.containerBottom.anchors.margins
            }

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
                Component.onCompleted: setSource("vehicle/VehicleItem.qml",{"vehicle": apx.vehicles.local, "z": z})
                onLoaded: {
                    item.z=200
                    map.addMapItem(item)
                }
            }


            Loader {
                active: showVehicles
                asynchronous: true
                Component.onCompleted: setSource("vehicle/VehicleItem.qml",{"vehicle": apx.vehicles.replay, "z": z})
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
            Loader {
                active: showVehicleNav
                asynchronous: true
                sourceComponent: Component { CamTargetCircleCmd { } }
                onLoaded: map.addMapItem(item)
            }
            Loader {
                active: showVehicleNav
                asynchronous: true
                sourceComponent: Component { CamTargetCircle { } }
                onLoaded: map.addMapItemGroup(item)
            }

            Loader { //travel path current
                active: showVehicleNav
                asynchronous: true
                source: "vehicle/TravelPath.qml"
                onLoaded: map.addMapItem(item)
            }


            Connections {
                enabled: showVehicles && showVehicleNav
                target: apx.vehicles.current.mission
                onTriggered: map.showRegion(apx.vehicles.current.mission.boundingGeoRectangle())
            }
        }
    }

    //Controls
    Component.onCompleted: {
        ui.main.mainLayout.addToolInfo(wind, Qt.AlignLeft|Qt.AlignBottom)
        //ui.main.mainLayout.addItem(busy,Qt.AlignRight|Qt.AlignBottom)
        ui.main.mainLayout.addInfo(info)
        //ui.main.mainLayout.addInfo(scale)
    }
    Loader {
        id: wind
        active: showWind && m.windSpd.value>0
        asynchronous: true
        sourceComponent: Component { Wind { } }
        visible: status===Loader.Ready
    }
    /*Loader {
        id: busy
        active: showScale
        asynchronous: true
        sourceComponent: Component { MapBusy { } }
    }
    Loader {
        id: scale
        active: showScale
        asynchronous: true
        sourceComponent: Component { MapScale { } }
        //onLoaded: ui.main.mainLayout.addInfo(item)
    }*/
    Loader {
        id: info
        active: showInfo
        asynchronous: true
        sourceComponent: Component { MapInfo { } }
        //onLoaded: ui.main.mainLayout.addInfo(item)
    }

}
