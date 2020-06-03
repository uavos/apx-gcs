import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import Apx.Map.Common 1.0

MapPolyline {
    opacity: ui.effects?0.8:1
    line.width: replay?1.5:1.5
    line.color: replay
                ? Style.cBlue
                : vehicle.telemetry.active
                  ? Style.cLineGreen
                  : Style.cBlue

    property var vehicle: apx.vehicles.current

    property bool replay: vehicle.protocol.isReplay

    onVehicleChanged: updatePath()

    Connections {
        target: vehicle
        function onGeoPathAppend(p){ addCoordinate(p) }
    }
    Connections {
        enabled: replay
        target: vehicle
        function onGeoPathChanged(){ updatePath() }
    }
    Connections {
        target: vehicle.telemetry.rpath
        function onTriggered(){ setPath(QtPositioning.path()) }
    }

    function updatePath()
    {
        setPath(vehicle.geoPath)
    }

    function showRegion()
    {
        if(path.length>0){
            map.showRegion(vehicle.geoPathRect())
        }
    }


    Connections {
        enabled: replay
        target: apx.vehicles.replay.telemetry.reader
        function onTriggered(){ showRegion() }
    }
    Component.onCompleted: updatePath()
}
