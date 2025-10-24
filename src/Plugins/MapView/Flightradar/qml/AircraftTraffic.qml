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
import QtPositioning

MapItemGroup {
    id: aircraftTraffic
    z: 200

    property var markers: ({})
    property var map: ui.map


    Component {
        id: markerComponent
        AircraftMarker { }
    }

    Connections {
        target: QAT

        function onAircraftUpdated(callsign) {
        
            const air = QAT.getAircraft(callsign)
            if (!air) {
                return;
            }

            var marker = markers[callsign]

            if (!marker) {
                marker = markerComponent.createObject(ui.map, {
                    callsign: air.callsign,
                    lat: air.latitude / 1e7,
                    lon: air.longitude / 1e7,
                    altitude: air.altitude,
                })

                map.addMapItem(marker)

                if (!marker) {
                    console.warn("Failed to create marker for", callsign)
                    return
                }

                markers[callsign] = marker
                console.log("Created marker:", air.callsign, marker.coordinate)
            } else {
                marker.lat = air.latitude / 1e7
                marker.lon = air.longitude / 1e7
                marker.altitude = air.altitude
                //marker.heading = air.heading
            }

            //printAllMarkers();
            //console.log("Update marker:", air.callsign, marker.coordinate)
        }

        function onAircraftRemoved(callsign) {
            const marker = markers[callsign]
            if (marker) {
                marker.destroy()
                delete markers[callsign]
                console.log("Marker removed:", callsign)
            }
        }
    }

    function printAllMarkers() {
        for (let key in markers) {
            const m = markers[key];
            console.log("Callsign:", m.callsign, "Lat:", m.lat, "Lon:", m.lon, "Heading:", m.heading);
        }
    }

    /*
    AircraftMarker {
        lat: 0.6
        lon: 0.6
        callsign: "TEST"
    }

    MapQuickItem {
        id: master
        z: 500
        coordinate: QtPositioning.coordinate(0.5, 0.5)
        sourceItem: Rectangle {
            width: 8
            height: 8
            radius: width / 2
            color: "#f00"
            border.color: "#fff"
            border.width: 2
        }
        Behavior on coordinate {
            enabled: true
            CoordinateAnimation { duration: 500; easing.type: Easing.InOutQuad }
        }
    }
    */
}






















//AircraftTraffic.qml

/*

import QtQuick
import QtLocation
import QtPositioning

import Apx.Common
import Apx.Application

MapItemGroup {
    id: aircraftTraffic
    z: 400

    property var markers: ({})  

    Component { 
        id: markerComponent
        AircraftMarker { }
    }

    Connections {
        target: qat

        function onAircraftUpdated(callsign) {
            
            const air = qat.getAircraft(callsign)
            if (!air) {
                return;
            }

            let marker = markers[callsign]

            console.log("Marker:", air, marker)

            
            if (!marker) {
                marker = markerComponent.createObject(ui.map, {
                    callsign: air.callsign,
                    lat: air.latitude / 1e7,
                    lon: air.longitude / 1e7,
                    //heading: air.heading
                })

                if (!marker) {
                    //console.log("ERROR: Marker creation failed for", callsign)
                    return;
                }

                markers[callsign] = marker
                //console.log("Marker created:", callsign, marker.lat, marker.lon)
            } else {
                marker.lat = air.latitude / 1e7
                marker.lon = air.longitude / 1e7
                //marker.heading = air.heading
                //console.log("Marker updated:", callsign, marker.lat, marker.lon)
            }

            printAllMarkers()
        }

        function onAircraftRemoved(callsign) {
            const marker = markers[callsign]
            if (marker) {
                marker.destroy()
                delete markers[callsign]
                console.log("Marker removed:", callsign)
            }
        }
    }

    function printAllMarkers() {
        for (let key in markers) {
            const m = markers[key];
            console.log("Callsign:", m.callsign, "Lat:", m.lat, "Lon:", m.lon, "Heading:", m.heading);
        }
    }

    AircraftMarker {
        lat: 24.2571
        lon: 54.2625
        callsign: "TEST"
    }

    MapQuickItem {
        id: master
        z: 500
        coordinate: QtPositioning.coordinate(0.05, 0.05)
        sourceItem: Rectangle {
            width: 8
            height: 8
            radius: width / 2
            color: "#f00"
            border.color: "#fff"
            border.width: 2
        }
        Behavior on coordinate {
            enabled: true
            CoordinateAnimation { duration: 500; easing.type: Easing.InOutQuad }
        }
    }
}
*/
