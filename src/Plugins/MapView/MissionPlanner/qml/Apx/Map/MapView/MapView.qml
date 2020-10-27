/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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
                        var mt=mapBase.supportedMapTypes[i]
                        vtypes.push(mt.description)
                    }
                    mapPlugin.prefs.maptype.enumStrings=vtypes
                    //mapPlugin.prefs.maptype.value=activeMapType.description
                }
                property string mapTypeName: mapPlugin.prefs.maptype.text
                activeMapType: {
                    var v = mapTypeName
                    for(var i in mapBase.supportedMapTypes){
                        var mt=mapBase.supportedMapTypes[i]
                        if(mt.description !== v) continue
                        return mt
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
                function onTriggered(){ map.showRegion(apx.vehicles.current.mission.boundingGeoRectangle()) }
            }
        }
    }
}
