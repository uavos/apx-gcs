import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12

import ".."

MapPolyline {
    z: 50
    opacity: ui.effects?0.8:1
    line.width: replay?4:2
    line.color: replay
                ? Style.cBlue
                : vehicle.telemetry.active
                  ? Style.cLineGreen
                  : Style.cBlue

    property var vehicle: apx.vehicles.current

    property bool replay: vehicle.isReplay()

    onVehicleChanged: updatePath()

    Connections {
        target: vehicle
        onGeoPathAppend: addCoordinate(p)
    }
    Connections {
        enabled: replay
        target: vehicle
        onGeoPathChanged: updatePath()
    }
    Connections {
        target: vehicle.action.rpath
        onTriggered: setPath(QtPositioning.path())
    }

    function updatePath()
    {
        setPath(vehicle.geoPath)
        if(replay)showRegion()
    }

    function showRegion()
    {
        if(path.length>0){
            var r=vehicle.geoPathRect()
            r.width*=1.5
            r.height*=1.5
            map.showRegion(r)
        }
    }


    Connections {
        enabled: replay
        target: apx.vehicles.REPLAY.telemetry.reader
        onTriggered: showRegion()
    }
    Component.onCompleted: updatePath()
}
