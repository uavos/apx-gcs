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
import QtQuick
import QtLocation

import QtQuick.Controls

import Apx.Map.Vehicles
import Apx.Map.Mission
import Apx.Map.Navigation

Control {
    id: control

    property bool showVehicles: true
    property bool showMission: showVehicles
    property bool showNavigation: showVehicles

    property bool showWind: showNavigation
    property bool showScale: showNavigation
    property bool showInfo: showNavigation


    property alias map: mapView

    signal mapBackgroundItemLoaded(var item)

    //internal
    property var mapPlugin: apx.tools.missionplanner

    background: Item {
        id: mapTilesItem
        anchors.fill: parent
        layer.enabled: ui.effects
        layer.effect: ShaderEffect {
            fragmentShader: Qt.resolvedUrl("/shaders/vignette.frag.qsb")
        }

        MapItemsOverlay {
            id: mapView
            anchors.fill: parent

            focus: false

            color: "#333"
            copyrightsVisible: false

            Component.onCompleted: {
                // assign plugin after map is created

                // collect available providers
                var plugin = Qt.createQmlObject(`import QtLocation 5.12; Plugin {}`, mapView)
                mapPlugin.prefs.provider.enumStrings=plugin.availableServiceProviders
                plugin.destroy()

                plugin = Qt.createQmlObject(`
                    import QtLocation 5.12
                    Plugin {
                        preferred: [
                            mapPlugin.prefs.provider.text.trim(),
                            "osm"
                        ]
                    }
                `, mapView)


                mapView.plugin=plugin

                // collect available map types
                var vtypes=[]
                for(var i in mapView.supportedMapTypes){
                    var mt=mapView.supportedMapTypes[i]
                    vtypes.push(mt.description)
                }
                mapPlugin.prefs.maptype.enumStrings=vtypes
            }

            activeMapType: {
                var v = mapPlugin.prefs.maptype.text
                for(var i in mapView.supportedMapTypes){
                    var mt=mapView.supportedMapTypes[i]
                    if(mt.description !== v) continue
                    return mt
                }
                return mapView.supportedMapTypes[0]
            }



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
                function onTriggered(){ mapView.showRegion(apx.vehicles.current.mission.boundingGeoRectangle()) }
            }
        }

    }
}
